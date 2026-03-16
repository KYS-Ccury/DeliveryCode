// : 서버 구현부이다.

#include "Server.h"
#include "Utils.h"
#include "ConnectionManager.h"

#include <iostream>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>

Server::Server(int port, int workerCount)
    : m_port(port),
      m_router(m_sessionManager, m_orderManager, m_dbManager),
      m_workerPool(workerCount, m_router) {
}

// : 서버를 시작한다.
void Server::start() {
    // : DB 연결을 먼저 해 둔다.
    m_dbManager.connect("demo_connection_string");

    // : 리슨 소켓을 초기화한다.
    initServerSocket();

    // : epoll을 초기화한다.
    initEpoll();

    // : 워커 스레드들을 시작한다.
    m_workerPool.start();

    // : 이벤트 버퍼를 준비한다.
    epoll_event events[64];

    std::cout << "[SERVER] started on port " << m_port << std::endl;

    // : 메인 이벤트 루프이다.
    while (true) {
        // : epoll 이벤트를 기다린다.
        int count = epoll_wait(m_epollFd, events, 64, -1);
        if (count < 0) {
            // : epoll_wait 실패 시 예외를 던진다.
            throw std::runtime_error("epoll_wait failed");
        }

        // : 발생한 이벤트들을 순회한다.
        for (int i = 0; i < count; ++i) {
            int fd = events[i].data.fd;

            // : 리슨 소켓이면 새 연결을 받는다.
            if (fd == m_listenFd) {
                acceptClient();
            }
            else {
                // : 일반 클라이언트 소켓이면 읽기 처리를 한다.
                handleReadable(fd);
            }
        }
    }
}

// : 서버 리슨 소켓을 초기화한다.
void Server::initServerSocket() {
    // : TCP 소켓을 생성한다.
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0) {
        throw std::runtime_error("socket failed");
    }

    // : 재사용 옵션을 켠다.
    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // : non-blocking 모드로 바꾼다.
    setNonBlocking(m_listenFd);

    // : 바인딩 주소 구조체를 만든다.
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    // : 포트에 바인딩한다.
    if (bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind failed");
    }

    // : 리슨 상태로 전환한다.
    if (listen(m_listenFd, SOMAXCONN) < 0) {
        throw std::runtime_error("listen failed");
    }
}

// : epoll을 초기화한다.
void Server::initEpoll() {
    // : epoll 인스턴스를 생성한다.
    m_epollFd = epoll_create1(0);
    if (m_epollFd < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }

    // : 리슨 소켓을 epoll에 등록한다.
    epoll_event ev {};
    ev.events = EPOLLIN;
    ev.data.fd = m_listenFd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &ev) < 0) {
        throw std::runtime_error("epoll_ctl add listen fd failed");
    }
}

// : 새 클라이언트를 accept 한다.
void Server::acceptClient() {
    while (true) {
        // : 클라이언트를 accept 한다.
        int clientFd = accept(m_listenFd, nullptr, nullptr);
        if (clientFd < 0) {
            // : 더 이상 받을 연결이 없으면 루프를 종료한다.
            break;
        }

        // : 클라이언트 소켓도 non-blocking 모드로 설정한다.
        setNonBlocking(clientFd);

        // : 세션을 등록한다.
        m_sessionManager.addSession(clientFd);

        // : epoll에 클라이언트 소켓을 등록한다.
        epoll_event ev {};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = clientFd;

        epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientFd, &ev);

        std::cout << "[CONNECT] fd=" << clientFd << std::endl;
    }
}

// : 클라이언트 읽기 이벤트를 처리한다.
void Server::handleReadable(int clientFd) {
    // : 세션을 조회한다.
    auto session = m_sessionManager.getSession(clientFd);
    if (!session) {
        return;
    }

    char buffer[1024];

    while (true) {
        // : 소켓에서 데이터를 읽는다.
        ssize_t n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        // : 정상 종료이면 연결을 정리한다.
        if (n == 0) {
            std::cout << "[DISCONNECT] fd=" << clientFd << std::endl;
            close(clientFd);
            m_sessionManager.removeSession(clientFd);
            return;
        }

        // : 음수이면 더 읽을 데이터가 없거나 에러이다.
        if (n < 0) {
            break;
        }

        // : 문자열 끝을 붙인다.
        buffer[n] = '\0';

        // : 세션 버퍼에 누적한다.
        {
            std::lock_guard<std::mutex> lock(session->mtx);
            session->readBuffer += buffer;

            // : 개행 단위로 패킷을 분리한다.
            std::size_t pos;
            while ((pos = session->readBuffer.find('\n')) != std::string::npos) {
                // : 한 줄을 잘라낸다.
                std::string line = session->readBuffer.substr(0, pos);

                // : 처리한 부분을 버퍼에서 제거한다.
                session->readBuffer.erase(0, pos + 1);

                // : 빈 줄은 무시한다.
                if (line.empty()) {
                    continue;
                }

                // : 문자열을 Packet으로 파싱한다.
                Packet packet = parsePacket(clientFd, line);

                // : 워커 큐에 작업을 넣는다.
                m_workerPool.enqueue(packet);
            }
        }
    }
}