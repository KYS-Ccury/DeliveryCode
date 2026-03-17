//  주문 관련 요청을 처리하는 핸들러이다.

#pragma once

#include "IHandler.h"
#include "../domain/OrderManager.h"
#include "../infra/DatabaseManager.h"
#include "../server/SessionManager.h"

class OrderHandler : public IHandler {
public:
    //  생성자이다.
    OrderHandler(OrderManager& orderManager, DatabaseManager& dbManager, SessionManager& sessionManager);

    //  주문 요청을 처리한다.
    void handle(const Packet& packet) override;

private:
    //  주문 매니저 참조이다.
    OrderManager& m_orderManager;

    //  DB 매니저 참조이다.
    DatabaseManager& m_dbManager;

    //  세션 매니저 참조이다.
    SessionManager& m_sessionManager;
};