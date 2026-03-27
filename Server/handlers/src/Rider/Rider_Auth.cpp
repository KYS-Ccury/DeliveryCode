#include "RiderHandler.h"
#include "RiderDB.h"
#include "CommonDB.h"
#include "Protocol.h"
#include "Session.h"
#include <iostream>

using json = nlohmann::json;

// ================================================================
//  onSignup
//  CommonDB(BaseHandler)가 users 테이블에 INSERT 한 뒤 호출된다.
//  userId 는 이미 확정된 값이므로 여기서는 RiderDB 위임만 수행한다.
// ================================================================
void RiderHandler::onSignup(Session* session, int userId, const json& reqBody) {
    try {
        std::string vehicleType = reqBody.value("vehicle_type", "BIKE");

        RiderDB::getInstance().insertRiderProfile(userId, vehicleType);

        json res;
        res["status"] = Status::SUCCESS;
        sendResponse(session, CmdCommon::REQ_SIGNUP, res);

    } catch (const std::exception& e) {
        std::cerr << "[Rider] onSignup 에러: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_SIGNUP,
                  Status::SERVER_ERROR, "라이더 프로필 생성 중 오류 발생");
    }
}

// ================================================================
//  onLoginSuccess
//  → 프로필 조회 + 온라인 상태 전환
// ================================================================
void RiderHandler::onLoginSuccess(Session* session, int userId, const json&) {
    try {
        auto& rdb = RiderDB::getInstance();

        rdb.insertRiderProfileIfMissing(userId);

        auto profile = rdb.queryRiderProfile(userId);
        if (!profile.found) {
            sendError(session, CmdCommon::REQ_LOGIN,
                      Status::NOT_FOUND, "라이더 프로필 정보를 찾을 수 없습니다.");
            return;
        }

        rdb.setOnline(userId, true);

        json res;
        res["status"]       = Status::SUCCESS;
        res["rider_id"]     = userId;
        res["name"]         = profile.name;
        res["phone"]        = profile.phone;
        res["vehicle_type"] = profile.vehicleType;
        res["is_working"]   = profile.isWorking;
        res["is_accepting"] = profile.isAccepting;
        sendResponse(session, CmdCommon::REQ_LOGIN, res);

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_LOGIN,
                  Status::SERVER_ERROR, "라이더 로그인 처리 중 오류 발생");
    }
}

// ================================================================
//  onLogout
//  → RiderDB::setOnline(false)
// ================================================================
void RiderHandler::onLogout(Session* session, int userId) {
    try {
        RiderDB::getInstance().setOnline(userId, false);
        RiderDB::getInstance().setWorkStatus(userId, "OFFLINE");

        json res;
        res["status"] = Status::SUCCESS;
        sendResponse(session, CmdCommon::REQ_LOGOUT, res);

    } catch (...) {
        sendError(session, CmdCommon::REQ_LOGOUT,
                  Status::SERVER_ERROR, "로그아웃 처리 중 오류");
    }
}

// ================================================================
//  onGetProfile  (REQ_GET_PROFILE = 104)
//  action 분기 → 각 RiderDB / CommonDB 메서드로 위임
// ================================================================
void RiderHandler::onGetProfile(Session* session, int userId, const json& req) {
    try {
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
            if (!CommonDB::getInstance().queryCheckPassword(userId, curPw)) {
                sendError(session, CmdCommon::REQ_GET_PROFILE,
                          Status::UNAUTHORIZED, "현재 비밀번호가 올바르지 않습니다.");
                return;
            }
            CommonDB::getInstance().queryChangePassword(userId, newPw);

            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = "CHANGE_PW";
            sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);
            return;
        }

        // ── 계좌 정보 변경 ───────────────────────────────────
        if (action == "CHANGE_ACCT") {
            std::string bank    = req.value("bank",    "");
            std::string holder  = req.value("holder",  "");
            std::string account = req.value("account", "");

            if (bank.empty() || holder.empty() || account.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE,
                          Status::BAD_REQUEST, "계좌 정보를 모두 입력하세요.");
                return;
            }
            RiderDB::getInstance().changeAccountInfo(userId, bank, holder, account);

            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = "CHANGE_ACCT";
            sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);
            return;
        }

        // ── 차량 종류 변경 ───────────────────────────────────
        if (action == "VEHICLE") {
            std::string vt = req.value("vehicle_type", "BIKE");
            RiderDB::getInstance().changeVehicleType(userId, vt);

            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = "VEHICLE";
            sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);
            return;
        }

        // ── 회원가입 직후 차량 업데이트 ──────────────────────
        if (req.contains("vehicle_type") && action.empty()) {
            RiderDB::getInstance().changeVehicleType(
                userId, req.value("vehicle_type", "BIKE"));
        }

        // ── 기본 프로필 조회 ─────────────────────────────────
        auto profile = RiderDB::getInstance().queryRiderProfile(userId);
        if (!profile.found) {
            sendError(session, CmdCommon::REQ_GET_PROFILE,
                      Status::NOT_FOUND, "사용자 없음");
            return;
        }

        json res;
        res["status"]       = Status::SUCCESS;
        res["name"]         = profile.name;
        res["phone"]        = profile.phone;
        res["vehicle_type"] = profile.vehicleType;
        res["is_working"]   = profile.isWorking;
        sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_GET_PROFILE,
                  Status::SERVER_ERROR, e.what());
    }
}
