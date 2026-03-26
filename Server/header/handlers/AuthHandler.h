//  ?몄쬆 愿???붿껌??泥섎━?섎뒗 ?몃뱾?ъ씠??

#pragma once

#include "IHandler.h"
#include "../server/SessionManager.h"

class AuthHandler : public IHandler {
public:
    //  ?앹꽦?먯씠??
    explicit AuthHandler(SessionManager& sessionManager);

    //  濡쒓렇???붿껌??泥섎━?쒕떎.
    void handle(const Packet& packet) override;

private:
    //  ?몄뀡 留ㅻ땲? 李몄“?대떎.
    SessionManager& m_sessionManager;
};