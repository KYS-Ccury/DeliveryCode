// Owner_Store.cpp
#include "OwnerHandler.h"
#include "MiddleHandler.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// 메뉴 목록 조회 (313)
void OwnerHandler::handleMenuList(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_MENU_LIST, req);
        sendResponse(session, CmdOwner::REQ_MENU_LIST, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_MENU_LIST, Status::SERVER_ERROR, "조회 실패");
    }
}

// 메뉴 등록 (302)
void OwnerHandler::handleAddMenu(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_ADD_MENU, req);
        sendResponse(session, CmdOwner::REQ_ADD_MENU, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_ADD_MENU, Status::SERVER_ERROR, "등록 실패");
    }
}

// 메뉴 수정 (303)
void OwnerHandler::handleUpdateMenu(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_UPDATE_MENU, req);
        sendResponse(session, CmdOwner::REQ_UPDATE_MENU, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_UPDATE_MENU, Status::SERVER_ERROR, "수정 실패");
    }
}

// 메뉴 삭제 (312)
void OwnerHandler::handleDeleteMenu(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_DEL_MENU, req);
        sendResponse(session, CmdOwner::REQ_DEL_MENU, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_DEL_MENU, Status::SERVER_ERROR, "삭제 실패");
    }
}

void OwnerHandler::handleUpdateStatus(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        
        // 1316번 프로토콜로 DB 업데이트 요청
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_UPDATE_STATUS, req);
        
        sendResponse(session, CmdOwner::REQ_UPDATE_STATUS, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_UPDATE_STATUS, Status::SERVER_ERROR, "상태 변경 실패");
    }
}