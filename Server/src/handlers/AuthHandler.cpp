//  ?몄쬆 ?몃뱾??援ы쁽遺?대떎.

#include "AuthHandler.h"
#include "../server/ConnectionManager.h"

AuthHandler::AuthHandler(SessionManager& sessionManager)
    : m_sessionManager(sessionManager) {
}

//  濡쒓렇???붿껌??泥섎━?쒕떎.
void AuthHandler::handle(const Packet& packet) {
    //  ?몄뀡??媛?몄삩??
    auto session = m_sessionManager.getSession(packet.clientFd);
    if (!session) {
        //  ?몄뀡???놁쑝硫?醫낅즺?쒕떎.
        return;
    }

    //  ?덉젣?먯꽌??payload瑜?洹몃?濡??ъ슜???대쫫泥섎읆 痍④툒?쒕떎.
    std::lock_guard<std::mutex> lock(session->mtx);

    //  ?곕え?⑹쑝濡?userId瑜?1濡?怨좎젙?쒕떎.
    session->userId = 1;
    session->isLoggedIn = true;

    //  濡쒓렇???깃났 ?묐떟??蹂대궦??
    ConnectionManager::sendToClient(packet.clientFd, "AUTH_OK");
}