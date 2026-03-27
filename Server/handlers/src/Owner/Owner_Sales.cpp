// Owner_Sales.cpp
#include "OwnerHandler.h"
#include "MiddleHandler.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

void OwnerHandler::handleSalesStats(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        
        // 1308번 DB 프로토콜로 매출 조회 요청
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_SALES_STATS, req);
        
        sendResponse(session, CmdOwner::REQ_SALES_STATS, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_SALES_STATS, Status::SERVER_ERROR, "매출 조회 실패");
    }
}