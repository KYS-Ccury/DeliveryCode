// EpollServer.cpp 갱신본 – getSessionByUserID 추가
// 기존 setupServer / start / acceptConnection 코드는 유지하고 아래 내용만 추가/수정

#include "EpollServer.h"
#include "Session.h"
#include "ThreadPool.h"
#include "Dispatcher.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

// 정적 인스턴스 포인터 정의
EpollServer* EpollServer::s_instance = nullptr;

EpollServer::EpollServer(int port, ThreadPool* pool)
    : port(port), server_fd(-1), epoll_fd(-1), pool(pool)
{}

EpollServer::~EpollServer()
{
    std::lock_guard<std::mutex> lock(session_mutex);
    for (auto& kv : sessions) delete kv.second;
    sessions.clear();
    if (server_fd != -1) close(server_fd);
    if (epoll_fd  != -1) close(epoll_fd);
}

void EpollServer::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool EpollServer::setupServer()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { std::cerr << "[EpollServer] socket 생성 실패\n"; return false; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "[EpollServer] Bind 실패 (port=" << port << ")\n"; return false;
    }
    if (listen(server_fd, SOMAXCONN) == -1) {
        std::cerr << "[EpollServer] Listen 실패\n"; return false;
    }

    setNonBlocking(server_fd);
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) { std::cerr << "[EpollServer] epoll_create1 실패\n"; return false; }

    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) return false;

    return true;
}

void EpollServer::acceptConnection()
{
    sockaddr_in caddr{};
    socklen_t   clen = sizeof(caddr);
    while (true) {
        int cfd = accept(server_fd, (sockaddr*)&caddr, &clen);
        if (cfd == -1) break;

        setNonBlocking(cfd);
        epoll_event ev{};
        ev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT;
        ev.data.fd = cfd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev);

        std::lock_guard<std::mutex> lock(session_mutex);
        sessions[cfd] = new Session(cfd);
        std::cout << "[EpollServer] 클라이언트 연결: fd=" << cfd << std::endl;
    }
}

void EpollServer::rearmEpoll(int fd)
{
    epoll_event ev{};
    ev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void EpollServer::start()
{
    if (!setupServer()) {
        std::cerr << "[EpollServer] 서버 설정 실패\n";
        return;
    }
    std::cout << "[EpollServer] 포트 " << port << " 에서 대기 중...\n";

    constexpr int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                acceptConnection();
            } else {
                // 워커 스레드에 처리 위임
                pool->enqueue([this, fd]() {
                    Session* session = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(session_mutex);
                        auto it = sessions.find(fd);
                        if (it == sessions.end()) return;
                        session = it->second;
                    }
                    bool alive = session->readFromSocket(pool);
                    if (!alive) {
                        std::cout << "[EpollServer] 클라이언트 종료: fd=" << fd << "\n";
                        std::lock_guard<std::mutex> lock(session_mutex);
                        delete sessions[fd];
                        sessions.erase(fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    } else {
                        rearmEpoll(fd);
                    }
                });
            }
        }
    }
}

// ─────────────────────────────────────────────────
// Push 알림 전송 시 고객 세션 조회
// ─────────────────────────────────────────────────
Session* EpollServer::getSessionByUserID(int userID)
{
    std::lock_guard<std::mutex> lock(session_mutex);
    for (auto& kv : sessions) {
        if (kv.second->getUserID() == userID)
            return kv.second;
    }
    return nullptr;
}
