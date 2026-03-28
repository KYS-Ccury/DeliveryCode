#include "OwnerHandler.h"
#include "MiddleHandler.h" 
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

void OwnerHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
    std::cout << "[Owner] 사장님 로그인 성공! UserID: " << userId << "\n";

    json res;
    res["status"] = Status::SUCCESS;
    res["message"] = "로그인에 성공했습니다.";
    
    json storeReq = {{"owner_id", userId}};
    json storeRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_STORE_INFO, storeReq);
    
    if (storeRes["status"] == Status::SUCCESS) {
        res["store_info"] = storeRes; 
    }

    sendResponse(session, CmdCommon::REQ_LOGIN, res);
}

void OwnerHandler::onSignup(Session* session, int userId, const nlohmann::json& reqBody) {
    std::cout << "[Owner] 사장님 특화 가입 처리 (ID: " << userId << ")\n";

    // 🚨 [핵심 수정] 클라이언트가 보낸 reqBody(전화번호, 주소 등)를 그대로 복사합니다!
    json createReq = reqBody; 
    createReq["owner_id"] = userId;
    createReq["action"] = "CREATE"; 

    json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_UPDATE_STORE, createReq);

    if (dbRes["status"] == Status::SUCCESS) {
        json res;
        res["status"] = Status::SUCCESS;
        res["message"] = reqBody.value("store_name", "가게") + " 사장님, 회원가입이 완료되었습니다.";
        sendResponse(session, CmdCommon::REQ_SIGNUP, res);
    } else {
        sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "매장 정보 생성 실패");
    }
}

void OwnerHandler::onLogout(Session* session, int userId) {
    std::cout << "[Owner] 사장님 로그아웃 (ID: " << userId << ")\n";
}

void OwnerHandler::onGetProfile(Session* session, int userId, const json& reqBody) {
}