// CommonHandler.cpp
#include "CommonHandler.h"
#include "CommonDB.h"       
#include "RiderDB.h" 
#include "Protocol.h"
#include "MiddleHandler.h"

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


void CommonHandler::handleSignup(Session* session, const json& reqBody) {
    int clientType = reqBody.value("client_type", 1);

    // 1. 내부 프로토콜을 사용해 MiddleHandler로 DB 요청 (users 테이블 삽입)
    // 🚨 (수정) DB 직접 접근이 아닌 MiddleHandler 라우팅 사용!
    json dbRes = MiddleHandler::processDBRequest(CmdDBCommon::REQ_DB_SIGNUP, reqBody);

    if (dbRes["status"] == Status::SUCCESS) {
        int userId = dbRes.value("user_id", -1);

        // 2. 클라이언트 타입에 따른 하위 테이블 데이터 생성 (Hook)
        // 🚨 (수정) 4가지 역할 모두 추가 및 MiddleHandler 경유 처리
        
        if (clientType == (int)ClientType::OWNER) {
            // [사장님] restaurants 테이블에 매장 초기 데이터 생성 (1301번 프로토콜 재활용)
            json hookReq = reqBody;
            hookReq["owner_id"] = userId;
            hookReq["action"] = "CREATE"; 
            MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_UPDATE_STORE, hookReq);
        } 
        else if (clientType == (int)ClientType::RIDER) {
            // [라이더] riders 테이블에 초기 프로필 생성
            // MiddleHandler::processDBRequest(CmdDBRider::REQ_DB_CREATE_PROFILE, {{"user_id", userId}});
        }
        else if (clientType == (int)ClientType::CUSTOMER) {
            // [고객] 장바구니 테이블 초기화 또는 기본 배달지 설정 (필요 시)
            // MiddleHandler::processDBRequest(CmdDBCustomer::REQ_DB_CREATE_PROFILE, {{"user_id", userId}});
        }
        else if (clientType == (int)ClientType::ADMIN) {
            // [관리자] 관리자 전용 권한 테이블 초기 셋팅 (필요 시)
            // MiddleHandler::processDBRequest(CmdDBAdmin::REQ_DB_CREATE_PROFILE, {{"user_id", userId}});
        }

        // 클라이언트에게 성공 응답 전송
        sendResponse(session, clientType, CmdCommon::REQ_SIGNUP, dbRes);
    } else {
        // 실패 (중복 아이디 등) 에러 메시지 전송
        std::string errMsg = dbRes.value("message", "Signup Failed");
        sendError(session, clientType, CmdCommon::REQ_SIGNUP, dbRes["status"], errMsg);
    }
}

// CommonHandler.cpp 의 handleLogin 함수 내부 일부 수정

void CommonHandler::handleLogin(Session* session, const json& reqBody) {
    int clientType = reqBody.value("client_type", 1);

    // 1. 내부 프로토콜을 사용해 MiddleHandler로 로그인 DB 검증 요청
    json dbRes = MiddleHandler::processDBRequest(CmdDBCommon::REQ_DB_LOGIN, reqBody);

    if (dbRes["status"] == Status::SUCCESS) {
        // DB 응답에 client_type이 있다면 갱신
        if (dbRes.count("client_type")) {
            clientType = dbRes["client_type"];
        }
        int userId = dbRes["user_id"];

        json finalRes = dbRes; // 최종 클라이언트에게 보낼 응답

        // 2. 클라이언트 타입에 맞는 추가 정보 로드 (내부 프로토콜 사용)
        if (clientType == (int)ClientType::OWNER) {
            // [사장님] 로그인 시 내 매장 정보를 가져오는 DB 프로토콜 호출
            json storeRes = MiddleHandler::processDBRequest(CmdDBOwner::REQ_DB_STORE_INFO, {{"owner_id", userId}});
            if (storeRes["status"] == Status::SUCCESS) {
                finalRes["store_info"] = storeRes; 
            }
        } 
        else if (clientType == (int)ClientType::RIDER) {
            // [라이더] 로그인 시 현재 내 배달 상태나 프로필 정보 로드
            // json riderRes = MiddleHandler::processDBRequest(CmdDBRider::REQ_DB_GET_RIDER_PROFILE, {{"user_id", userId}});
            // if (riderRes["status"] == Status::SUCCESS) { finalRes["rider_info"] = riderRes; }
        }
        // 🚨 [추가됨] 고객 로그인 처리
        else if (clientType == (int)ClientType::CUSTOMER) {
            // [고객] 로그인 시 기본 배달 주소지나 닉네임 등을 로드
            // json customerRes = MiddleHandler::processDBRequest(CmdDBCustomer::REQ_DB_CUSTOMER_INFO, {{"user_id", userId}});
            // if (customerRes["status"] == Status::SUCCESS) { finalRes["customer_info"] = customerRes; }
        }
        // 🚨 [추가됨] 관리자 로그인 처리
        else if (clientType == (int)ClientType::ADMIN) {
            // [관리자] 로그인 시 관리자 권한 레벨 등을 로드
            // json adminRes = MiddleHandler::processDBRequest(CmdDBAdmin::REQ_DB_ADMIN_INFO, {{"user_id", userId}});
            // if (adminRes["status"] == Status::SUCCESS) { finalRes["admin_info"] = adminRes; }
        }

        // 최종 응답 전송
        sendResponse(session, clientType, CmdCommon::REQ_LOGIN, finalRes);
    } else {
        // 로그인 실패 (비밀번호 틀림 등)
        sendError(session, clientType, CmdCommon::REQ_LOGIN, dbRes["status"], dbRes.value("message", "Login failed"));
    }
}