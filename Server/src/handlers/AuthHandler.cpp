//  인증 핸들러 구현부이다.

#include "AuthHandler.h"
#include "../server/ConnectionManager.h"

AuthHandler::AuthHandler(SessionManager& sessionManager)
    : m_sessionManager(sessionManager) {
}

//  로그인 요청을 처리한다.
void AuthHandler::handle(const Packet& packet) {
    //  세션을 가져온다.
    auto session = m_sessionManager.getSession(packet.clientFd);
    if (!session) {
        //  세션이 없으면 종료한다.
        return;
    }

    //  예제에서는 payload를 그대로 사용자 이름처럼 취급한다.
    std::lock_guard<std::mutex> lock(session->mtx);

    //  데모용으로 userId를 1로 고정한다.
    session->userId = 1;
    session->isLoggedIn = true;

    //  로그인 성공 응답을 보낸다.
    ConnectionManager::sendToClient(packet.clientFd, "AUTH_OK");
}