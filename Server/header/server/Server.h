//  ?쒕쾭 ?꾩껜瑜?愿由ы븯???듭떖 ?대옒?ㅼ씠??
//  accept, epoll, read, worker enqueue瑜??대떦?쒕떎.

#pragma once

#include <string>
#include "../domain/OrderManager.h"
#include "../infra/DatabaseManager.h"
#include "SessionManager.h"
#include "Router.h"
#include "WorkerPool.h"

class Server {
public:
    //  ?앹꽦?먯씠??
    Server(int port, int workerCount);

    //  ?쒕쾭瑜??쒖옉?쒕떎.
    void start();

private:
    //  ?쒕쾭 珥덇린???⑥닔?대떎.
    void initServerSocket();

    //  epoll 珥덇린???⑥닔?대떎.
    void initEpoll();

    //  ???대씪?댁뼵?몃? accept ?쒕떎.
    void acceptClient();

    //  ?쎄린 ?대깽?몃? 泥섎━?쒕떎.
    void handleReadable(int clientFd);

private:
    //  由ъ뒯 ?ы듃 踰덊샇?대떎.
    int m_port = 0;

    //  由ъ뒯 ?뚯폆 fd?대떎.
    int m_listenFd = -1;

    //  epoll fd?대떎.
    int m_epollFd = -1;

    //  ?몄뀡 留ㅻ땲??대떎.
    SessionManager m_sessionManager;

    //  二쇰Ц 留ㅻ땲??대떎.
    OrderManager m_orderManager;

    //  DB 留ㅻ땲??대떎.
    DatabaseManager m_dbManager;

    //  ?쇱슦?곗씠??
    Router m_router;

    //  ?뚯빱 ??대떎.
    WorkerPool m_workerPool;
};