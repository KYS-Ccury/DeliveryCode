// Server/src/router/Router.cpp

#include "../../header/router/Router.h"
#include "../../header/Dispatcher/Dispatcher.h"

void Router::Route(Session* session, Packet& pkt)
{
    dispatcher_->Dispatch(session, pkt);
}