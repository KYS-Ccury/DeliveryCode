#include "OwnerHandler.h"
#include "MiddleHandler.h" // 🚨 [수정] MiddleHandler 사용
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

void OwnerHandler::handleOrderList(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        int ownerId = req.value("owner_id", -1);

        if (ownerId == -1) {
            sendError(session, CmdOwner::REQ_ORDER_LIST, Status::BAD_REQUEST, "잘못된 요청입니다.");
            return;
        }

        // 🚨 [수정] MiddleHandler를 통해 DB에 주문 목록 요청 (1304 프로토콜)
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_ORDER_LIST, req);
        
        // 성공적으로 가져오면 응답 전송
        sendResponse(session, CmdOwner::REQ_ORDER_LIST, dbRes);
        
    } catch (const std::exception& e) {
        std::cerr << "[OwnerHandler] 주문 목록 조회 오류: " << e.what() << std::endl;
        sendError(session, CmdOwner::REQ_ORDER_LIST, Status::SERVER_ERROR, "서버 오류");
    }
}

// Owner_order.cpp 맨 아래에 추가
void OwnerHandler::handleAcceptOrder(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_ACCEPT_ORDER, req);
        sendResponse(session, CmdOwner::REQ_ACCEPT_ORDER, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_ACCEPT_ORDER, Status::SERVER_ERROR, "수락 처리 오류");
    }
}

void OwnerHandler::handleRejectOrder(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_REJECT_ORDER, req);
        sendResponse(session, CmdOwner::REQ_REJECT_ORDER, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_REJECT_ORDER, Status::SERVER_ERROR, "거절 처리 오류");
    }
}