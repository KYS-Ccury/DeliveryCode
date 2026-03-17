//  패킷 타입에 따라 알맞은 핸들러로 요청을 분기하는 라우터이다.

#pragma once

#include <memory>
#include "../handlers/IHandler.h"
#include "../handlers/AuthHandler.h"
#include "../handlers/OrderHandler.h"
#include "../handlers/ChatHandler.h"
#include "../handlers/RiderHandler.h"
#include "../domain/OrderManager.h"
#include "../infra/DatabaseManager.h"
#include "SessionManager.h"
#include "Packet.h"

class Router {
public:
    //  생성자이다.
    Router(SessionManager& sessionManager, OrderManager& orderManager, DatabaseManager& dbManager);

    //  패킷을 라우팅한다.
    void route(const Packet& packet);

private:
    //  각 기능별 핸들러 객체이다.
    std::unique_ptr<AuthHandler>  m_authHandler;
    std::unique_ptr<OrderHandler> m_orderHandler;
    std::unique_ptr<ChatHandler>  m_chatHandler;
    std::unique_ptr<RiderHandler> m_riderHandler;
};