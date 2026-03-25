//  二쇰Ц ?몃뱾??援ы쁽遺?대떎.

#include "OrderHandler.h"
#include "../server/ConnectionManager.h"
#include <string>

OrderHandler::OrderHandler(OrderManager& orderManager, DatabaseManager& dbManager, SessionManager& sessionManager)
    : m_orderManager(orderManager), m_dbManager(dbManager), m_sessionManager(sessionManager) {
}

//  二쇰Ц ?붿껌??泥섎━?쒕떎.
void OrderHandler::handle(const Packet& packet) {
    //  ?몄뀡??議고쉶?쒕떎.
    auto session = m_sessionManager.getSession(packet.clientFd);
    if (!session) {
        //  ?몄뀡???놁쑝硫?泥섎━?섏? ?딅뒗??
        return;
    }

    //  濡쒓렇???щ?瑜??뺤씤?쒕떎.
    {
        std::lock_guard<std::mutex> lock(session->mtx);
        if (!session->isLoggedIn) {
            ConnectionManager::sendToClient(packet.clientFd, "ORDER_FAIL|NOT_LOGIN");
            return;
        }
    }

    //  二쇰Ц???앹꽦?쒕떎.
    int orderId = m_orderManager.createOrder(session->userId, packet.payload);

    //  DB 濡쒓렇瑜??④릿??
    m_dbManager.saveOrderLog(orderId, "ORDER_CREATED");

    //  二쇰Ц ?앹꽦 ?깃났 ?묐떟??蹂대궦??
    ConnectionManager::sendToClient(packet.clientFd, "ORDER_OK|" + std::to_string(orderId));
}