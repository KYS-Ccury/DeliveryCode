// Server/src/Dispatcher/Dispatcher.cpp

#include "../../header/Dispatcher/Dispatcher.h"
#include <iostream>

void Dispatcher::Dispatch(Session* session, Packet& pkt)
{
    switch (pkt.type)
    {
    case PKT_LOGIN:
        authHandler_->Handle(session, pkt);
        break;

    case PKT_CHAT:
        chatHandler_->Handle(session, pkt);
        break;

    default:
        std::cout << "[Dispatcher] Unknown packet type\n";
        break;
    }
}