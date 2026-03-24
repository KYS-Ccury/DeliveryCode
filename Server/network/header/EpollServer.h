#pragma once
#include <unordered_map>
#include <mutex>
#include <sys/epoll.h>
#include "Session.h"

class ThreadPool;

class EpollServer {
public:
    // OwnerHandler / RiderHandler에서 Push 전송 시 고객 세션 조회용
    static EpollServer* s_instance;

    EpollServer(int port, ThreadPool* pool);
    ~EpollServer();

    bool setupServer();
    void start();         // epoll_wait 루프
    void acceptConnection();

    // userID로 Session 포인터 조회 (로그인된 세션만 반환)
    Session* getSessionByUserID(int userID);

private:
    void setNonBlocking(int fd);
    void rearmEpoll(int fd);

    int         port;
    int         server_fd;
    int         epoll_fd;
    ThreadPool* pool;

    std::mutex                        session_mutex;
    std::unordered_map<int, Session*> sessions; // fd → Session*
};
