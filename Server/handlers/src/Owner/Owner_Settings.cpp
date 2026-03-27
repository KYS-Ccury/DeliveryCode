#include "OwnerHandler.h"
#include "MiddleHandler.h"
#include "Protocol.h"

using json = nlohmann::json;

void OwnerHandler::handleGetSettings(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_GET_SETTINGS, req);
        sendResponse(session, CmdOwner::REQ_GET_SETTINGS, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_GET_SETTINGS, Status::SERVER_ERROR, "설정 조회 실패");
    }
}

void OwnerHandler::handleUpdateSettings(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_UPDATE_SETTINGS, req);
        sendResponse(session, CmdOwner::REQ_UPDATE_SETTINGS, dbRes);
    } catch (...) {
        sendError(session, CmdOwner::REQ_UPDATE_SETTINGS, Status::SERVER_ERROR, "설정 수정 실패");
    }
}