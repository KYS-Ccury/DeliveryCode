//  주문 핸들러 구현부이다.

#include "OrderHandler.h"
#include "../server/ConnectionManager.h"
#include <string>

OrderHandler::OrderHandler(OrderManager& orderManager, DatabaseManager& dbManager, SessionManager& sessionManager)
    : m_orderManager(orderManager), m_dbManager(dbManager), m_sessionManager(sessionManager) {
}

//  주문 요청을 처리한다.
void OrderHandler::handle(const Packet& packet) {
    //  세션을 조회한다.
    auto session = m_sessionManager.getSession(packet.clientFd);
    if (!session) {
        //  세션이 없으면 처리하지 않는다.
        return;
    }

    //  로그인 여부를 확인한다.
    {
        std::lock_guard<std::mutex> lock(session->mtx);
        if (!session->isLoggedIn) {
            ConnectionManager::sendToClient(packet.clientFd, "ORDER_FAIL|NOT_LOGIN");
            return;
        }
    }

    //  주문을 생성한다.
    int orderId = m_orderManager.createOrder(session->userId, packet.payload);

    //  DB 로그를 남긴다.
    m_dbManager.saveOrderLog(orderId, "ORDER_CREATED");

    //  주문 생성 성공 응답을 보낸다.
    ConnectionManager::sendToClient(packet.clientFd, "ORDER_OK|" + std::to_string(orderId));
}