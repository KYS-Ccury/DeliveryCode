//  라우터 구현부이다.

#include "Router.h"
#include "ConnectionManager.h"

Router::Router(SessionManager& sessionManager, OrderManager& orderManager, DatabaseManager& dbManager) {
    //  핸들러들을 생성한다.
    m_authHandler  = std::make_unique<AuthHandler>(sessionManager);
    m_orderHandler = std::make_unique<OrderHandler>(orderManager, dbManager, sessionManager);
    m_chatHandler  = std::make_unique<ChatHandler>();
    m_riderHandler = std::make_unique<RiderHandler>();
}

//  패킷 타입에 따라 알맞은 핸들러로 보낸다.
void Router::route(const Packet& packet) {
    //  패킷 타입별 분기 처리이다.
    switch (packet.type) {
    case PacketType::AUTH_LOGIN:
        m_authHandler->handle(packet);
        break;

    case PacketType::ORDER_CREATE:
        m_orderHandler->handle(packet);
        break;

    case PacketType::CHAT_SEND:
        m_chatHandler->handle(packet);
        break;

    case PacketType::RIDER_UPDATE:
        m_riderHandler->handle(packet);
        break;

    default:
        //  알 수 없는 요청이면 에러 응답을 보낸다.
        ConnectionManager::sendToClient(packet.clientFd, "ERR|UNKNOWN_PACKET");
        break;
    }
}