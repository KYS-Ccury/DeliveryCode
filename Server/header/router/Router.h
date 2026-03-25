//  ?⑦궥 ??낆뿉 ?곕씪 ?뚮쭪? ?몃뱾?щ줈 ?붿껌??遺꾧린?섎뒗 ?쇱슦?곗씠??

#pragma once

#include <memory>
#include <unordered_map>
#include "../handlers/IHandler.h"
#include "../handlers/AuthHandler.h"
#include "../handlers/OrderHandler.h"
#include "../handlers/ChatHandler.h"
#include "../handlers/RiderHandler.h"
#include "../domain/OrderManager.h"
#include "../infra/DatabaseManager.h"
#include "SessionManager.h"
#include "Packet.h"

class Router {
public:
    //  ?앹꽦?먯씠??
    Router(SessionManager& sessionManager, OrderManager& orderManager, DatabaseManager& dbManager);

    //  ?⑦궥???쇱슦?낇븳??
    void route(const Packet& packet);

private:
    //  ?⑦궥 ??낅퀎 ?몃뱾??留ㅽ븨?대떎.
    std::unordered_map<PacketType, IHandler*> m_routes;

    //  媛?湲곕뒫蹂??몃뱾??媛앹껜?대떎.
    std::unique_ptr<AuthHandler>  m_authHandler;
    std::unique_ptr<OrderHandler> m_orderHandler;
    std::unique_ptr<ChatHandler>  m_chatHandler;
    std::unique_ptr<RiderHandler> m_riderHandler;
};