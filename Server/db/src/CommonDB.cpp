#include "CommonDB.h"
#include "MariaDB_AcceptManager.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// ───────────────────────────────────────────────────────────────────
// ★ 단일 진입점: MiddleHandler에서 전달받은 프로토콜을 각 함수로 라우팅
// ───────────────────────────────────────────────────────────────────
json CommonDB::process(uint16_t dbProtocol, const json& reqJson) {
    try {
        switch (dbProtocol) {
            case CmdDBCommon::REQ_DB_SIGNUP:
                return signup(reqJson);
            case CmdDBCommon::REQ_DB_LOGIN:
                return login(reqJson);
            case CmdDBCommon::REQ_DB_GET_PROFILE:
                // 프로필 조회 로직이 필요하다면 여기에 추가
                return {{"status", Status::SUCCESS}};
            default:
                return {{"status", Status::SERVER_ERROR}, {"message", "알 수 없는 공통 DB 프로토콜"}};
        }
    } catch (const std::exception& e) {
        std::cerr << "[CommonDB] Exception: " << e.what() << std::endl;
        return {{"status", Status::SERVER_ERROR}, {"message", "DB 핸들러 내부 오류"}};
    }
}

// ───────────────────────────────────────────────────────────────────
// 1. 공통 회원가입
// ───────────────────────────────────────────────────────────────────
json CommonDB::signup(const json& reqJson) {
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    
    // SQL 인젝션 방지 처리
    std::string id = db.escapeStr(reqJson.value("id", ""));
    std::string pw = db.escapeStr(reqJson.value("pw", ""));
    int clientType = reqJson.value("client_type", 1); 
    
    // users 테이블 NOT NULL 제약조건 대응 (클라이언트가 안 보냈을 경우 기본값)
    std::string name = db.escapeStr(reqJson.value("name", "이름없음"));
    std::string phone = db.escapeStr(reqJson.value("phone", "010-0000-0000"));

    // client_type(1~4)을 스키마의 ENUM('CUSTOMER','OWNER','RIDER','ADMIN')으로 변환
    std::string roleStr;
    switch(clientType) {
        case 1: roleStr = "CUSTOMER"; break;
        case 2: roleStr = "OWNER"; break;
        case 3: roleStr = "RIDER"; break;
        case 4: roleStr = "ADMIN"; break;
        default: roleStr = "CUSTOMER"; break;
    }

    // 1. ID 중복 검사
    auto check = db.executeQuery("SELECT user_id FROM users WHERE login_id='" + id + "'");
    if (!check.empty()) {
        res["status"] = Status::BAD_REQUEST;
        res["message"] = "이미 존재하는 아이디입니다.";
        return res;
    }

    // 2. users 테이블에 공통 정보 Insert
    std::string query = "INSERT INTO users (login_id, password, role, name, phone) VALUES ('" + 
                        id + "', '" + pw + "', '" + roleStr + "', '" + name + "', '" + phone + "')";
    
    if (db.executeUpdate(query)) {
        res["status"] = Status::SUCCESS;
        res["user_id"] = db.getLastInsertId(); // 방금 가입한 유저의 PK 반환
        res["client_type"] = clientType;       // 후처리(Hook) 라우팅을 위해 반환
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "회원가입 실패 (DB Insert 오류)";
    }
    return res;
}

// ───────────────────────────────────────────────────────────────────
// 2. 공통 로그인
// ───────────────────────────────────────────────────────────────────
json CommonDB::login(const json& reqJson) {
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    
    std::string id = db.escapeStr(reqJson.value("id", ""));
    std::string pw = db.escapeStr(reqJson.value("pw", ""));

    // users 테이블에서 검증 (role도 가져오기)
    std::string query = "SELECT user_id, role FROM users WHERE login_id='" + id + "' AND password='" + pw + "'";
    auto rows = db.executeQuery(query);

    if (rows.empty()) {
        res["status"] = Status::UNAUTHORIZED;
        res["message"] = "아이디 또는 비밀번호가 일치하지 않습니다.";
        return res;
    }

    // DB의 ENUM 문자열을 다시 통신용 client_type 정수로 변환하여 응답
    std::string roleStr = rows[0].at("role");
    int clientType = 1; // CUSTOMER
    if (roleStr == "OWNER") clientType = 2;
    else if (roleStr == "RIDER") clientType = 3;
    else if (roleStr == "ADMIN") clientType = 4;

    res["status"] = Status::SUCCESS;
    res["user_id"] = std::stoi(rows[0].at("user_id"));
    res["client_type"] = clientType;
    return res;
}