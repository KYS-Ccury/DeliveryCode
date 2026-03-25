#include "RiderHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include "Session.h"
#include <iostream>

using json = nlohmann::json;

// ================================================================
//  onSignup  (Basehandler::handleSignup 에서 users INSERT 완료 후 호출)
//  rider_profiles INSERT: 클라이언트가 vehicle_type 전달 시 반영
// ================================================================
void RiderHandler::onSignup(Session* session, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();
        std::string loginId = reqBody.value("id", "");

        auto rows = db.executeQuery(
            "SELECT user_id FROM users WHERE login_id='" +
            escapeStr(loginId) + "' LIMIT 1");

        if (!rows.empty()) {
            int uid = std::stoi(rows[0].at("user_id"));

            // 클라이언트가 vehicle_type 전달 시 반영, 없으면 기본 BIKE
            std::string vt = escapeStr(reqBody.value("vehicle_type", "BIKE"));

            db.executeUpdate(
                "INSERT INTO rider_profiles "
                "(user_id, vehicle_type, is_working, is_accepting, is_online) "
                "VALUES (" + std::to_string(uid) + ",'" + vt + "',FALSE,FALSE,FALSE)");
        }

        json res; res["status"] = Status::SUCCESS;
        sendResponse(session, CmdCommon::REQ_SIGNUP, res);

    } catch (const std::exception& e) {
        std::cerr << "[Rider] onSignup 에러: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_SIGNUP,
                  Status::SERVER_ERROR, "라이더 프로필 생성 중 오류 발생");
    }
}

// ================================================================
//  onLoginSuccess
//  응답: { status, rider_id, name, phone, vehicle_type,
//           is_working, is_accepting }
// ================================================================
void RiderHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();

        auto rows = db.executeQuery(
            "SELECT u.name, u.phone, "
            "       rp.vehicle_type, rp.is_working, rp.is_accepting "
            "FROM users u "
            "LEFT JOIN rider_profiles rp ON rp.user_id = u.user_id "
            "WHERE u.user_id=" + std::to_string(userId));

        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN,
                      Status::NOT_FOUND, "라이더 프로필 정보를 찾을 수 없습니다.");
            return;
        }

        // rider_profiles 없으면 자동 생성
        auto& row = rows[0];
        if (!row.count("vehicle_type") || row.at("vehicle_type").empty()) {
            db.executeUpdate(
                "INSERT IGNORE INTO rider_profiles "
                "(user_id, vehicle_type, is_working, is_accepting, is_online) "
                "VALUES (" + std::to_string(userId) + ",'BIKE',FALSE,FALSE,FALSE)");
        }

        // 온라인 상태로 변경
        db.executeUpdate(
            "UPDATE rider_profiles SET is_online=TRUE WHERE user_id=" +
            std::to_string(userId));

        json res;
        res["status"]       = Status::SUCCESS;
        res["rider_id"]     = userId;
        res["name"]         = row.at("name");
        res["phone"]        = row.count("phone")        && !row.at("phone").empty()
                                ? row.at("phone") : "";
        res["vehicle_type"] = row.count("vehicle_type") && !row.at("vehicle_type").empty()
                                ? row.at("vehicle_type") : "BIKE";
        res["is_working"]   = (row.count("is_working")  && row.at("is_working")  == "1");
        res["is_accepting"] = (row.count("is_accepting") && row.at("is_accepting") == "1");

        sendResponse(session, CmdCommon::REQ_LOGIN, res);

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_LOGIN,
                  Status::SERVER_ERROR, "라이더 로그인 처리 중 오류 발생");
    }
}

// ================================================================
//  onLogout
// ================================================================
void RiderHandler::onLogout(Session* session, int userId) {
    try {
        auto& db = MariaDBManager::getInstance();
        db.executeUpdate(
            "UPDATE rider_profiles "
            "SET is_online=FALSE, is_working=FALSE "
            "WHERE user_id=" + std::to_string(userId));

        json res; res["status"] = Status::SUCCESS;
        sendResponse(session, CmdCommon::REQ_LOGOUT, res);
    } catch (...) {
        sendError(session, CmdCommon::REQ_LOGOUT,
                  Status::SERVER_ERROR, "로그아웃 처리 중 오류");
    }
}

// ================================================================
//  onGetProfile  (REQ_GET_PROFILE = 104 / CMD_GET_MY_INFO)
//
//  클라이언트 action 분기:
//    "CHANGE_PW"   → 비밀번호 변경  { cur_pw, new_pw }
//    "CHANGE_ACCT" → 계좌 변경      { bank, holder, account }
//    "VEHICLE"     → 차량 변경      { vehicle_type }
//    (없음/기타)  → 프로필 조회
// ================================================================
void RiderHandler::onGetProfile(Session* session, int userId, const json& req) {
    try {
        auto& db = MariaDBManager::getInstance();
        std::string action = req.value("action", "");

        // ── 비밀번호 변경 ────────────────────────────────────
        if (action == "CHANGE_PW") {
            std::string curPw = req.value("cur_pw", "");
            std::string newPw = req.value("new_pw", "");

            if (curPw.empty() || newPw.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE,
                          Status::BAD_REQUEST, "현재/새 비밀번호를 입력하세요.");
                return;
            }

            // 현재 비밀번호 확인
            auto chk = db.executeQuery(
                "SELECT user_id FROM users WHERE user_id=" +
                std::to_string(userId) +
                " AND password='" + escapeStr(curPw) + "'");
            if (chk.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE,
                          Status::UNAUTHORIZED, "현재 비밀번호가 올바르지 않습니다.");
                return;
            }

            db.executeUpdate(
                "UPDATE users SET password='" + escapeStr(newPw) +
                "' WHERE user_id=" + std::to_string(userId));

            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = "CHANGE_PW";
            sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);
            return;
        }

        // ── 계좌 변경 ────────────────────────────────────────
        if (action == "CHANGE_ACCT") {
            std::string bank    = req.value("bank",    "");
            std::string holder  = req.value("holder",  "");
            std::string account = req.value("account", "");

            if (bank.empty() || holder.empty() || account.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE,
                          Status::BAD_REQUEST, "계좌 정보를 모두 입력하세요.");
                return;
            }

            // users 테이블에 계좌 정보 컬럼이 없으므로
            // rider_profiles의 확장 또는 별도 저장 필요.
            // 현재 스키마에는 없으므로 bank_name, account_holder, account_number를
            // license_no 컬럼을 재활용하거나 별도 처리.
            // → 여기서는 응답만 반환 (실제 DB 저장은 스키마 확장 후 처리)
            // TODO: rider_profiles에 bank_name, account_holder, account_number 컬럼 추가 후 저장
            std::cout << "[RiderAuth] CHANGE_ACCT: userId=" << userId
                      << " bank=" << bank << " holder=" << holder << std::endl;

            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = "CHANGE_ACCT";
            sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);
            return;
        }

        // ── 차량 종류 변경 ───────────────────────────────────
        if (action == "VEHICLE") {
            std::string vt = escapeStr(req.value("vehicle_type", "BIKE"));
            db.executeUpdate(
                "UPDATE rider_profiles SET vehicle_type='" + vt +
                "' WHERE user_id=" + std::to_string(userId));

            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = "VEHICLE";
            sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);
            return;
        }

        // ── vehicle_type만 있는 경우 (회원가입 직후 차량 업데이트) ──
        if (req.contains("vehicle_type") && action.empty()) {
            std::string vt = escapeStr(req.value("vehicle_type", "BIKE"));
            db.executeUpdate(
                "UPDATE rider_profiles SET vehicle_type='" + vt +
                "' WHERE user_id=" + std::to_string(userId));
        }

        // ── 기본 프로필 조회 ─────────────────────────────────
        auto rows = db.executeQuery(
            "SELECT u.name, u.phone, "
            "       rp.vehicle_type, rp.is_working "
            "FROM users u "
            "JOIN rider_profiles rp ON rp.user_id = u.user_id "
            "WHERE u.user_id=" + std::to_string(userId));

        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_GET_PROFILE,
                      Status::NOT_FOUND, "사용자 없음");
            return;
        }

        json res;
        res["status"]       = Status::SUCCESS;
        res["name"]         = rows[0].at("name");
        res["phone"]        = rows[0].count("phone")        ? rows[0].at("phone")        : "";
        res["vehicle_type"] = rows[0].count("vehicle_type") ? rows[0].at("vehicle_type") : "BIKE";
        res["is_working"]   = (rows[0].count("is_working")  && rows[0].at("is_working")  == "1");
        sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_GET_PROFILE,
                  Status::SERVER_ERROR, e.what());
    }
}
