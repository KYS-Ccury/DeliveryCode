//  二쇰Ц 愿???붿껌??泥섎━?섎뒗 ?몃뱾?ъ씠??

#pragma once

#include "IHandler.h"
#include "../domain/OrderManager.h"
#include "../infra/DatabaseManager.h"
#include "../server/SessionManager.h"

class OrderHandler : public IHandler {
public:
    //  ?앹꽦?먯씠??
    OrderHandler(OrderManager& orderManager, DatabaseManager& dbManager, SessionManager& sessionManager);

    //  二쇰Ц ?붿껌??泥섎━?쒕떎.
    void handle(const Packet& packet) override;

private:
    //  二쇰Ц 留ㅻ땲? 李몄“?대떎.
    OrderManager& m_orderManager;

    //  DB 留ㅻ땲? 李몄“?대떎.
    DatabaseManager& m_dbManager;

    //  ?몄뀡 留ㅻ땲? 李몄“?대떎.
    SessionManager& m_sessionManager;
};