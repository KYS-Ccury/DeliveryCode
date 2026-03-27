#include "OwnerHandler.h"
#include "MiddleHandler.h" // 🚨 [수정] OwnerDB 직접 참조 대신 MiddleHandler 사용!
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

void OwnerHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
    std::cout << "[Owner] 사장님 로그인 성공! UserID: " << userId << "\n";

    json res;
    res["status"] = Status::SUCCESS;
    res["message"] = "로그인에 성공했습니다.";
    
    // 🚨 [수정] MiddleHandler를 통해 DB에 매장 정보 요청 (1300 프로토콜)
    json storeReq = {{"owner_id", userId}};
    json storeRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_STORE_INFO, storeReq);
    
    if (storeRes["status"] == Status::SUCCESS) {
        res["store_info"] = storeRes; // 가져온 매장 정보를 클라이언트 응답에 추가
    }

    sendResponse(session, CmdCommon::REQ_LOGIN, res);
}

void OwnerHandler::onSignup(Session* session, int userId, const nlohmann::json& reqBody) {
    std::cout << "[Owner] 사장님 특화 가입 처리 (ID: " << userId << ")\n";

    std::string storeName = reqBody.value("store_name", "이름없는 가게");

    // 🚨 [수정] MiddleHandler를 통해 DB에 매장 생성 요청 (1301 프로토콜 재사용)
    json createReq = {
        {"owner_id", userId},
        {"store_name", storeName},
        {"action", "CREATE"} // 삽입 모드임을 알림
    };
    json dbRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_UPDATE_STORE, createReq);

    if (dbRes["status"] == Status::SUCCESS) {
        json res;
        res["status"] = Status::SUCCESS;
        res["message"] = storeName + " 사장님, 회원가입이 완료되었습니다.";
        sendResponse(session, CmdCommon::REQ_SIGNUP, res);
    } else {
        sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "매장 정보 생성 실패");
    }
}

void OwnerHandler::onLogout(Session* session, int userId) {
    std::cout << "[Owner] 사장님 로그아웃 (ID: " << userId << ")\n";
}

void OwnerHandler::onGetProfile(Session* session, int userId, const json& reqBody) {
    // 사장님 상세 프로필 조회 로직
}