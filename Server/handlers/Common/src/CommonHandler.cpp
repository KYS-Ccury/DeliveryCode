// CommonHandler.cpp
#include "CommonHandler.h"
#include "CommonDB.h"       
#include "RiderDB.h" 
#include "Protocol.h"

using json = nlohmann::json;

// --- [추가됨] CommonHandler 전용 응답 송신 유틸리티 ---
static void sendResponse(Session* session, int clientType, uint16_t protocol, const json& payload) {
    if (session) {
        session->sendPacket(static_cast<uint8_t>(clientType), protocol, payload.dump());
    }
}

static void sendError(Session* session, int clientType, uint16_t protocol, int statusCode, const std::string& message) {
    json res;
    res["status"]  = statusCode;
    res["message"] = message;
    if (session) {
        session->sendPacket(static_cast<uint8_t>(clientType), protocol, res.dump());
    }
}
// ----------------------------------------------------

void CommonHandler::handleSignup(Session* session, const json& reqBody) {
    // 1. 공통 회원가입 처리 (users 테이블 삽입)
    json dbRes = CommonDB::signup(reqBody);
    
    // 클라이언트 타입 추출 (기본값 1)
    int clientType = reqBody.value("client_type", 1);

    if (dbRes["status"] == Status::SUCCESS) {
        // 2. 클라이언트 타입에 따른 하위 테이블 데이터 생성 (Hook)
        if (clientType == (int)ClientType::RIDER) {
            RiderDB::createProfile(dbRes["user_id"]);
        } else if (clientType == (int)ClientType::OWNER) {
            // OwnerDB::createProfile(dbRes["user_id"]);
        }
        
        // BaseHandler:: 가 아닌 상단에 만든 전용 함수 사용
        sendResponse(session, clientType, CmdCommon::REQ_SIGNUP, dbRes);
    } else {
        std::string errMsg = dbRes.count("message") ? dbRes["message"].get<std::string>() : "Signup Failed";
        sendError(session, clientType, CmdCommon::REQ_SIGNUP, dbRes["status"], errMsg);
    }
}

void CommonHandler::handleLogin(Session* session, const json& reqBody) {
    // 1. 공통 로그인 검증 (users 테이블 확인)
    json dbRes = CommonDB::login(reqBody);
    
    // 클라이언트 타입 추출
    int clientType = reqBody.value("client_type", 1);

    if (dbRes["status"] == Status::SUCCESS) {
        // DB 응답에 client_type이 있다면 갱신
        if (dbRes.count("client_type")) {
            clientType = dbRes["client_type"];
        }
        int userId = dbRes["user_id"];

        json finalRes = dbRes; // 최종 클라이언트에게 보낼 응답

        // 2. 클라이언트 타입에 맞는 추가 정보 로드 및 상태 변경
        if (clientType == (int)ClientType::RIDER) {
            finalRes = RiderDB::loginHook(userId); 
        } 
        
        sendResponse(session, clientType, CmdCommon::REQ_LOGIN, finalRes);
    } else {
        std::string errMsg = dbRes.count("message") ? dbRes["message"].get<std::string>() : "Login Failed";
        sendError(session, clientType, CmdCommon::REQ_LOGIN, dbRes["status"], errMsg);
    }
}