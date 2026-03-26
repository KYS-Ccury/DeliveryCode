// : ?쒕쾭 援ы쁽遺?대떎.

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

// : ?쒕쾭瑜??쒖옉?쒕떎.
void Server::start() {
    // : DB ?곌껐??癒쇱? ???붾떎.
    m_dbManager.connect("demo_connection_string");

    // : 由ъ뒯 ?뚯폆??珥덇린?뷀븳??
    initServerSocket();

    // : epoll??珥덇린?뷀븳??
    initEpoll();

    // : ?뚯빱 ?ㅻ젅?쒕뱾???쒖옉?쒕떎.
    m_workerPool.start();

    // : ?대깽??踰꾪띁瑜?以鍮꾪븳??
    epoll_event events[64];

    std::cout << "[SERVER] started on port " << m_port << std::endl;

    // : 硫붿씤 ?대깽??猷⑦봽?대떎.
    while (true) {
        // : epoll ?대깽?몃? 湲곕떎由곕떎.
        int count = epoll_wait(m_epollFd, events, 64, -1);
        if (count < 0) {
            // : epoll_wait ?ㅽ뙣 ???덉쇅瑜??섏쭊??
            throw std::runtime_error("epoll_wait failed");
        }

        // : 諛쒖깮???대깽?몃뱾???쒗쉶?쒕떎.
        for (int i = 0; i < count; ++i) {
            int fd = events[i].data.fd;

            // : 由ъ뒯 ?뚯폆?대㈃ ???곌껐??諛쏅뒗??
            if (fd == m_listenFd) {
                acceptClient();
            }
            else {
                // : ?쇰컲 ?대씪?댁뼵???뚯폆?대㈃ ?쎄린 泥섎━瑜??쒕떎.
                handleReadable(fd);
            }
        }
    }
}

// : ?쒕쾭 由ъ뒯 ?뚯폆??珥덇린?뷀븳??
void Server::initServerSocket() {
    // : TCP ?뚯폆???앹꽦?쒕떎.
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0) {
        throw std::runtime_error("socket failed");
    }

    // : ?ъ궗???듭뀡??耳좊떎.
    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // : non-blocking 紐⑤뱶濡?諛붽씔??
    setNonBlocking(m_listenFd);

    // : 諛붿씤??二쇱냼 援ъ“泥대? 留뚮뱺??
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    // : ?ы듃??諛붿씤?⑺븳??
    if (bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind failed");
    }

    // : 由ъ뒯 ?곹깭濡??꾪솚?쒕떎.
    if (listen(m_listenFd, SOMAXCONN) < 0) {
        throw std::runtime_error("listen failed");
    }
}

// : epoll??珥덇린?뷀븳??
void Server::initEpoll() {
    // : epoll ?몄뒪?댁뒪瑜??앹꽦?쒕떎.
    m_epollFd = epoll_create1(0);
    if (m_epollFd < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }

    // : 由ъ뒯 ?뚯폆??epoll???깅줉?쒕떎.
    epoll_event ev {};
    ev.events = EPOLLIN;
    ev.data.fd = m_listenFd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &ev) < 0) {
        throw std::runtime_error("epoll_ctl add listen fd failed");
    }
}

// : ???대씪?댁뼵?몃? accept ?쒕떎.
void Server::acceptClient() {
    while (true) {
        // : ?대씪?댁뼵?몃? accept ?쒕떎.
        int clientFd = accept(m_listenFd, nullptr, nullptr);
        if (clientFd < 0) {
            // : ???댁긽 諛쏆쓣 ?곌껐???놁쑝硫?猷⑦봽瑜?醫낅즺?쒕떎.
            break;
        }

        // : ?대씪?댁뼵???뚯폆??non-blocking 紐⑤뱶濡??ㅼ젙?쒕떎.
        setNonBlocking(clientFd);

        // : ?몄뀡???깅줉?쒕떎.
        m_sessionManager.addSession(clientFd);

        // : epoll???대씪?댁뼵???뚯폆???깅줉?쒕떎.
        epoll_event ev {};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = clientFd;

        epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientFd, &ev);

        std::cout << "[CONNECT] fd=" << clientFd << std::endl;
    }
}

// : ?대씪?댁뼵???쎄린 ?대깽?몃? 泥섎━?쒕떎.
void Server::handleReadable(int clientFd) {
    // : ?몄뀡??議고쉶?쒕떎.
    auto session = m_sessionManager.getSession(clientFd);
    if (!session) {
        return;
    }

    char buffer[1024];

    while (true) {
        // : ?뚯폆?먯꽌 ?곗씠?곕? ?쎈뒗??
        ssize_t n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        // : ?뺤긽 醫낅즺?대㈃ ?곌껐???뺣━?쒕떎.
        if (n == 0) {
            std::cout << "[DISCONNECT] fd=" << clientFd << std::endl;
            close(clientFd);
            m_sessionManager.removeSession(clientFd);
            return;
        }

        // : ?뚯닔?대㈃ ???쎌쓣 ?곗씠?곌? ?녾굅???먮윭?대떎.
        if (n < 0) {
            break;
        }

        // : 臾몄옄???앹쓣 遺숈씤??
        buffer[n] = '\0';

        // : ?몄뀡 踰꾪띁???꾩쟻?쒕떎.
        {
            std::lock_guard<std::mutex> lock(session->mtx);
            session->readBuffer += buffer;

            // : 媛쒗뻾 ?⑥쐞濡??⑦궥??遺꾨━?쒕떎.
            std::size_t pos;
            while ((pos = session->readBuffer.find('\n')) != std::string::npos) {
                // : ??以꾩쓣 ?섎씪?몃떎.
                std::string line = session->readBuffer.substr(0, pos);

                // : 泥섎━??遺遺꾩쓣 踰꾪띁?먯꽌 ?쒓굅?쒕떎.
                session->readBuffer.erase(0, pos + 1);

                // : 鍮?以꾩? 臾댁떆?쒕떎.
                if (line.empty()) {
                    continue;
                }

                // : 臾몄옄?댁쓣 Packet?쇰줈 ?뚯떛?쒕떎.
                Packet packet = parsePacket(clientFd, line);

                // : ?뚯빱 ?먯뿉 ?묒뾽???ｋ뒗??
                m_workerPool.enqueue(packet);
            }
        }
    }
}