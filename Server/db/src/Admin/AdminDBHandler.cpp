#include "AdminDB.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// DB 동시접근 보호용 뮤텍스 정의
std::mutex AdminDBHandler::admin_db_mutex;

// ============================================================
// process — 1500번대 DB 프로토콜 라우팅 진입점이다.
// dbProtocol 번호에 따라 적절한 DB 함수를 호출한다.
// ============================================================
json AdminDBHandler::process(uint16_t dbProtocol, const json& reqJson)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : AdminDB process() dbProtocol=" << dbProtocol << std::endl;
    std::cout << "-------------------------" << std::endl;

    switch (dbProtocol)
    {
    // ── 인증 관련 ──
    case CmdDBAdmin::REQ_DB_MONITOR_ORDERS:  return monitorOrders(reqJson);
    case CmdDBAdmin::REQ_DB_RIDER_STATUS:    return riderStatus(reqJson);
    case CmdDBAdmin::REQ_DB_FORCE_DISPATCH:  return forceDispatch(reqJson);
    case CmdDBAdmin::REQ_DB_FORCE_CANCEL:    return forceCancel(reqJson);
    case CmdDBAdmin::REQ_DB_MANAGE_REVIEW:   return manageReview(reqJson);

    default:
    {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : AdminDB 미처리 프로토콜 " << dbProtocol << std::endl;
        std::cout << "-------------------------" << std::endl;
        json err;
        err["status"]  = Status::BAD_REQUEST;
        err["message"] = "알 수 없는 DB 프로토콜";
        return err;
    }
    }
}

// ============================================================
// loginHook — 관리자 로그인 DB 조회를 수행한다.
// users 테이블에서 login_id, password, role='ADMIN' 확인.
// ============================================================
json AdminDBHandler::loginHook(const json& reqJson)
{
    std::lock_guard<std::mutex> lock(admin_db_mutex);

    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : DB 로그인 조회" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        auto& db = MariaDBManager::getInstance();
        std::string loginId  = reqJson.value("login_id", "");
        std::string password = reqJson.value("password", "");

        if (loginId.empty() || password.empty()) {
            json res;
            res["status"]  = Status::BAD_REQUEST;
            res["message"] = "login_id, password 필요";
            return res;
        }

        // DB에서 관리자 계정을 조회한다.
        std::string q =
            "SELECT user_id, login_id, role FROM users "
            "WHERE login_id = '" + loginId +
            "' AND password = '" + password +
            "' AND role = 'ADMIN' AND status = 'ACTIVE' LIMIT 1";

        DBResult rows = db.executeQuery(q);

        json res;
        if (rows.empty()) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 로그인 실패 (유저 없음)" << std::endl;
            std::cout << "-------------------------" << std::endl;
            res["status"]  = Status::UNAUTHORIZED;
            res["message"] = "정보가 올바르지 않거나 권한이 없습니다.";
        } else {
            res["status"]  = Status::SUCCESS;
            res["user_id"] = std::stoi(rows[0].at("user_id"));
        }
        return res;

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : DB 로그인 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        json res;
        res["status"]  = Status::SERVER_ERROR;
        res["message"] = e.what();
        return res;
    }
}

// ============================================================
// logoutHook — 관리자 로그아웃 DB 처리를 수행한다.
// 현재는 별도 DB 작업 없이 성공을 반환한다.
// ============================================================
json AdminDBHandler::logoutHook(const json& reqJson)
{
    std::lock_guard<std::mutex> lock(admin_db_mutex);

    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : DB 로그아웃 처리" << std::endl;
    std::cout << "-------------------------" << std::endl;

    json res;
    res["status"]  = Status::SUCCESS;
    res["message"] = "로그아웃 완료";
    return res;
}

// ============================================================
// getProfile — 관리자 프로필을 DB에서 조회한다.
// ============================================================
json AdminDBHandler::getProfile(const json& reqJson)
{
    std::lock_guard<std::mutex> lock(admin_db_mutex);

    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : DB 프로필 조회" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        auto& db = MariaDBManager::getInstance();
        int userId = reqJson.value("user_id", 0);

        if (userId <= 0) {
            json res;
            res["status"]  = Status::BAD_REQUEST;
            res["message"] = "user_id 필요";
            return res;
        }

        // DB에서 프로필 정보를 조회한다.
        std::string q = "SELECT user_id, login_id, role, status FROM users WHERE user_id = "
                        + std::to_string(userId) + " LIMIT 1";
        DBResult rows = db.executeQuery(q);

        json res;
        if (rows.empty()) {
            res["status"]  = Status::NOT_FOUND;
            res["message"] = "사용자 없음";
        } else {
            res["status"]   = Status::SUCCESS;
            res["user_id"]  = rows[0].count("user_id")  ? rows[0].at("user_id")  : "";
            res["login_id"] = rows[0].count("login_id") ? rows[0].at("login_id") : "";
            res["role"]     = rows[0].count("role")      ? rows[0].at("role")      : "";
        }
        return res;

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : DB 프로필 조회 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        json res;
        res["status"]  = Status::SERVER_ERROR;
        res["message"] = e.what();
        return res;
    }
}