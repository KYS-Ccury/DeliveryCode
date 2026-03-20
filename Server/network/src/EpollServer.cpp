#include "EpollServer.h"
#include "Session.h"
#include "ThreadPool.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>

EpollServer::EpollServer(int port, ThreadPool* pool) 
    : port(port), server_fd(-1), epoll_fd(-1), pool(pool) {
}

EpollServer::~EpollServer() {
    // 1. 모든 세션(클라이언트) 연결 종료 및 메모리 해제
    for (auto& pair : sessions) {
        delete pair.second; // Session 소멸자에서 close(client_fd) 호출됨
    }
    sessions.clear();

    // 2. 서버 소켓 및 epoll FD 닫기
    if (server_fd != -1) close(server_fd);
    if (epoll_fd != -1)  close(epoll_fd);
}

// 소켓을 논블로킹 모드로 변경 (epoll을 Edge Triggered로 쓸 때 필수)
void EpollServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "[EpollServer] fcntl F_GETFL 에러" << std::endl;
        return;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 서버 소켓 초기화 (생성, 바인딩, 리슨)
bool EpollServer::setupServer() {
    // 1. TCP 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[EpollServer] 소켓 생성 실패" << std::endl;
        return false;
    }

    // 2. 포트 재사용 설정 (서버 재시작 시 "Address already in use" 에러 방지)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 서버 주소 구조체 설정
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 모든 IPからの 접속 허용
    server_addr.sin_port = htons(port);       // 포트 번호 설정

    // 4. Bind (소켓에 주소 할당)
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "[EpollServer] Bind 실패 (포트: " << port << ")" << std::endl;
        return false;
    }

    // 5. Listen (수신 대기 상태로 전환)
    if (listen(server_fd, SOMAXCONN) == -1) {
        std::cerr << "[EpollServer] Listen 실패" << std::endl;
        return false;
    }

    // 6. 서버 소켓을 논블로킹으로 설정
    setNonBlocking(server_fd);

    // 7. epoll 인스턴스 생성
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "[EpollServer] epoll_create1 실패" << std::endl;
        return false;
    }

    // 8. 서버 소켓을 epoll에 등록 (EPOLLIN: 읽기 이벤트 감지)
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd; // 이벤트 발생 시 확인할 FD

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "[EpollServer] epoll_ctl (서버 소켓 등록) 실패" << std::endl;
        return false;
    }

    return true;
}

// 새로운 클라이언트의 연결 수락
void EpollServer::acceptConnection() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) break;

        setNonBlocking(client_fd);

        // ★ 핵심 1: EPOLLONESHOT 플래그 추가!
        epoll_event event;
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT; 
        event.data.fd = client_fd;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);

        // 뮤텍스로 보호하며 세션 추가
        {
            std::lock_guard<std::mutex> lock(session_mutex);
            sessions[client_fd] = new Session(client_fd);
        }
    }
}

// ★ 핵심 2: 워커 스레드가 처리를 끝낸 후, 다시 epoll 감시망에 넣는 함수
void EpollServer::rearmSocket(int client_fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd = client_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1) {
        closeConnection(client_fd); // 재등록 실패 시 끊어버림
    }
}

void EpollServer::closeConnection(int client_fd) {
    std::lock_guard<std::mutex> lock(session_mutex);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    
    auto it = sessions.find(client_fd);
    if (it != sessions.end()) {
        delete it->second;
        sessions.erase(it);
    }
}

void EpollServer::start() {
    setupServer();
    std::vector<epoll_event> events(MAX_EVENTS);

    while (true) {
        int num_events = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        
        for (int i = 0; i < num_events; ++i) {
            int active_fd = events[i].data.fd;

            if (active_fd == server_fd) {
                acceptConnection();
            } 
            else if (events[i].events & EPOLLIN) {
                // ★ 핵심 3: 메인 스레드는 읽지 않는다! 스레드 풀로 넘겨버린다.
                Session* session = nullptr;
                {
                    std::lock_guard<std::mutex> lock(session_mutex);
                    if (sessions.find(active_fd) != sessions.end()) {
                        session = sessions[active_fd];
                    }
                }

                if (session) {
                    // 워커 스레드가 백그라운드에서 읽기 시작함
                    pool->enqueue([this, session, active_fd]() {
                        // 세션에서 데이터를 다 읽었으면 (EAGAIN 도달 시 true 반환)
                        if (session->readFromSocket(this->pool)) {
                            this->rearmSocket(active_fd); // 다시 감시 활성화
                        } else {
                            this->closeConnection(active_fd); // 에러/종료 시 접속 끊기
                        }
                    });
                }
            }
        }
    }
}