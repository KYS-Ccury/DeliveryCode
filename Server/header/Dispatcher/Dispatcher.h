// Server/header/server/Dispatcher.h

#pragma once
#include "../../Common/Packet.h"
#include "Session.h"
#include "../handlers/AuthHandler.h"

class Dispatcher {
public:
    Dispatcher(AuthHandler* authHandler)
        : authHandler_(authHandler) {}

    void Dispatch(Session* session, Packet& pkt);

private:
    AuthHandler* authHandler_;
};