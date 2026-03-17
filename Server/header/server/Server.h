//  서버 전체를 관리하는 핵심 클래스이다.
//  accept, epoll, read, worker enqueue를 담당한다.

#pragma once

#include <string>
#include "../domain/OrderManager.h"
#include "../infra/DatabaseManager.h"
#include "SessionManager.h"
#include "Router.h"
#include "WorkerPool.h"

class Server {
public:
    //  생성자이다.
    Server(int port, int workerCount);

    //  서버를 시작한다.
    void start();

private:
    //  서버 초기화 함수이다.
    void initServerSocket();

    //  epoll 초기화 함수이다.
    void initEpoll();

    //  새 클라이언트를 accept 한다.
    void acceptClient();

    //  읽기 이벤트를 처리한다.
    void handleReadable(int clientFd);

private:
    //  리슨 포트 번호이다.
    int m_port = 0;

    //  리슨 소켓 fd이다.
    int m_listenFd = -1;

    //  epoll fd이다.
    int m_epollFd = -1;

    //  세션 매니저이다.
    SessionManager m_sessionManager;

    //  주문 매니저이다.
    OrderManager m_orderManager;

    //  DB 매니저이다.
    DatabaseManager m_dbManager;

    //  라우터이다.
    Router m_router;

    //  워커 풀이다.
    WorkerPool m_workerPool;
};