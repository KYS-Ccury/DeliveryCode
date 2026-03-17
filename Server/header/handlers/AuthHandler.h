//  인증 관련 요청을 처리하는 핸들러이다.

#pragma once

#include "IHandler.h"
#include "../server/SessionManager.h"

class AuthHandler : public IHandler {
public:
    //  생성자이다.
    explicit AuthHandler(SessionManager& sessionManager);

    //  로그인 요청을 처리한다.
    void handle(const Packet& packet) override;

private:
    //  세션 매니저 참조이다.
    SessionManager& m_sessionManager;
};