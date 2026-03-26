// CommonHandler.cpp
#include "CommonHandler.h"
#include "CommonDB.h"       // DB 로직 호출을 위해 포함
#include "RiderDBHandler.h" // 라이더 훅 처리를 위해 포함
#include "Protocol.h"

using json = nlohmann::json;

void CommonHandler::handleSignup(Session* session, const json& reqBody) {
    // 1. 공통 회원가입 처리 (users 테이블 삽입)
    json dbRes = CommonDB::signup(reqBody);

    if (dbRes["status"] == Status::SUCCESS) {
        int clientType = reqBody.value("client_type", 1);
        
        // 2. 클라이언트 타입에 따른 하위 테이블(profiles) 데이터 생성 (Hook 호출)
        if (clientType == (int)ClientType::RIDER) {
            // 라이더면 라이더 프로필 생성 로직 호출!
            RiderDBHandler::createProfile(dbRes["user_id"]);
        } else if (clientType == (int)ClientType::OWNER) {
            // 사장님이면 사장님 프로필 생성 로직 호출!
            // OwnerDBHandler::createProfile(dbRes["user_id"]);
        }
        
        sendResponse(session, CmdCommon::REQ_SIGNUP, dbRes);
    } else {
        sendError(session, CmdCommon::REQ_SIGNUP, dbRes["status"], dbRes["message"]);
    }
}

void CommonHandler::handleLogin(Session* session, const json& reqBody) {
    // 1. 공통 로그인 검증 (users 테이블 확인)
    json dbRes = CommonDB::login(reqBody);

    if (dbRes["status"] == Status::SUCCESS) {
        int clientType = dbRes["client_type"];
        int userId = dbRes["user_id"];

        json finalRes; // 최종 클라이언트에게 보낼 응답

        // 2. 클라이언트 타입에 맞는 추가 정보 로드 및 상태 변경
        if (clientType == (int)ClientType::RIDER) {
            // 라이더의 차량 정보, 온라인 상태 등을 가져옴
            finalRes = RiderDBHandler::loginHook(userId); 
        } 
        // else if (clientType == 고객, 사장 등등...)
        
        sendResponse(session, CmdCommon::REQ_LOGIN, finalRes);
    } else {
        sendError(session, CmdCommon::REQ_LOGIN, dbRes["status"], dbRes["message"]);
    }
}