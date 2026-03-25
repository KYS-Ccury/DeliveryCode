#include "RiderHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include "Session.h"
#include <iostream>

using json = nlohmann::json;

// ───────────────────────────────────────────────────────────────────
// 1. 라이더 회원가입 후처리 (onSignup Hook)
// ───────────────────────────────────────────────────────────────────
void RiderHandler::onSignup(Session* session, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();
        std::string loginId = reqBody.value("id", "");
        
        // 1. 가입된 유저 ID 조회
        auto rows = db.executeQuery("SELECT user_id FROM users WHERE login_id='" + escapeStr(loginId) + "' LIMIT 1");
        if (!rows.empty()) {
            int uid = std::stoi(rows[0].at("user_id"));
            
            // ★ 수정: 라이더는 rider_profiles 테이블에 기본 정보 생성
            // 배달 수단(vehicle_type) 등은 기본값으로 세팅
            db.executeUpdate("INSERT INTO rider_profiles (user_id, vehicle_type, is_working, is_accepting) "
                             "VALUES (" + std::to_string(uid) + ", 'BIKE', FALSE, FALSE)");
        }

        json res; 
        res["status"] = Status::SUCCESS;
        sendResponse(session, CmdCommon::REQ_SIGNUP, res);

    } catch (const std::exception& e) {
        std::cerr << "[Rider] onSignup 에러: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "라이더 프로필 생성 중 오류 발생");
    }
}

// ───────────────────────────────────────────────────────────────────
// 2. 라이더 로그인 성공 후처리 (onLoginSuccess Hook)
// ───────────────────────────────────────────────────────────────────
void RiderHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();

        // 라이더 전용 정보 조회
        std::string q = "SELECT u.name, u.phone, rp.vehicle_type, rp.is_working, rp.is_accepting "
                        "FROM users u JOIN rider_profiles rp ON rp.user_id = u.user_id "
                        "WHERE u.user_id = " + std::to_string(userId);
        
        auto rows = db.executeQuery(q);
        
        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::NOT_FOUND, "라이더 프로필 정보를 찾을 수 없습니다.");
            return;
        }

        const auto& row = rows[0];

        // 온라인 상태 업데이트
        db.executeUpdate("UPDATE rider_profiles SET is_online = TRUE WHERE user_id = " + std::to_string(userId));

        json res;
        res["status"]       = Status::SUCCESS;
        res["rider_id"]     = userId;
        res["name"]         = row.at("name");
        res["vehicle_type"] = row.at("vehicle_type");
        res["is_working"]   = (row.at("is_working") == "1");
        res["is_accepting"] = (row.at("is_accepting") == "1");

        sendResponse(session, CmdCommon::REQ_LOGIN, res);

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, "라이더 로그인 처리 중 오류 발생");
    }
}

// ───────────────────────────────────────────────────────────────────
// 3. 라이더 로그아웃 후처리 (onLogout Hook) - ★ 추가 (링킹 에러 방지)
// ───────────────────────────────────────────────────────────────────
void RiderHandler::onLogout(Session* session, int userId) {
    try {
        auto& db = MariaDBManager::getInstance();
        // 로그아웃 시 오프라인/미근무 상태로 변경
        db.executeUpdate("UPDATE rider_profiles SET is_online = FALSE, is_working = FALSE WHERE user_id = " + std::to_string(userId));
        
        json res;
        res["status"] = Status::SUCCESS;
        sendResponse(session, CmdCommon::REQ_LOGOUT, res);
    } catch (...) {
        sendError(session, CmdCommon::REQ_LOGOUT, Status::SERVER_ERROR, "로그아웃 처리 중 오류");
    }
}

// ───────────────────────────────────────────────────────────────────
// 4. 라이더 프로필 조회/수정 (onGetProfile Hook) - ★ 추가 (링킹 에러 방지)
// ───────────────────────────────────────────────────────────────────
void RiderHandler::onGetProfile(Session* session, int userId, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();
        
        // 정보 업데이트 로직 (필요 시)
        if (reqBody.contains("vehicle_type")) {
            db.executeUpdate("UPDATE rider_profiles SET vehicle_type = '" + escapeStr(reqBody.value("vehicle_type", "BIKE")) + 
                             "' WHERE user_id = " + std::to_string(userId));
        }

        auto rows = db.executeQuery(
            "SELECT u.name, u.phone, rp.vehicle_type, rp.is_working "
            "FROM users u JOIN rider_profiles rp ON rp.user_id = u.user_id "
            "WHERE u.user_id = " + std::to_string(userId));

        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_GET_PROFILE, Status::NOT_FOUND, "사용자 없음");
            return;
        }

        json res;
        res["status"] = Status::SUCCESS;
        res["name"] = rows[0].at("name");
        res["phone"] = rows[0].at("phone");
        res["vehicle_type"] = rows[0].at("vehicle_type");
        res["is_working"] = (rows[0].at("is_working") == "1");

        sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, e.what());
    }
}