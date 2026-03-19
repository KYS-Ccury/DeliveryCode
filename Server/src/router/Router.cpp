//  라우터 구현부이다.

#include "Router.h"
#include "ConnectionManager.h"

Router::Router(SessionManager& sessionManager, OrderManager& orderManager, DatabaseManager& dbManager) {
    //  핸들러들을 생성한다.
    m_authHandler  = std::make_unique<AuthHandler>(sessionManager);
    m_orderHandler = std::make_unique<OrderHandler>(orderManager, dbManager, sessionManager);
    m_chatHandler  = std::make_unique<ChatHandler>();
    m_riderHandler = std::make_unique<RiderHandler>();

    //  패킷 타입과 핸들러를 매핑한다.
    m_routes = {
        { PacketType::AUTH_LOGIN,    m_authHandler.get()  },
        { PacketType::ORDER_CREATE,  m_orderHandler.get() },
        { PacketType::CHAT_SEND,     m_chatHandler.get()  },
        { PacketType::RIDER_UPDATE,  m_riderHandler.get() }
    };
}

//  패킷 타입에 따라 알맞은 핸들러로 보낸다.
void Router::route(const Packet& packet) {
    //  패킷 타입별 매핑을 사용해 핸들러를 찾는다.
    auto it = m_routes.find(packet.type);
    if (it != m_routes.end() && it->second) {
        it->second->handle(packet);
    } else {
        //  알 수 없는 요청이면 에러 응답을 보낸다.
        ConnectionManager::sendToClient(packet.clientFd, "ERR|UNKNOWN_PACKET");
    }
}