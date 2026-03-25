#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"

using json = nlohmann::json;

// ================================================================
//  onSignup
//  Basehandler::handleSignup 에서 users INSERT 완료 후 호출됨
//  여기서는 customer_profiles 부가 테이블만 INSERT
// ================================================================
void CustomerHandler::onSignup(Session* session, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();
        std::string id = reqBody.value("id", "");

        // users 에서 방금 INSERT 된 user_id 조회
        auto rows = db.executeQuery(
            "SELECT user_id FROM users WHERE login_id='" +
            escapeStr(id) + "' LIMIT 1");

        if (!rows.empty()) {
            int uid = std::stoi(rows[0].at("user_id"));

            // customer_profiles INSERT (포인트 0으로 초기화)
            db.executeUpdate(
                "INSERT INTO customer_profiles (user_id, point) VALUES (" +
                std::to_string(uid) + ", 0)");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCommon::REQ_SIGNUP, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_SIGNUP,
                  Status::SERVER_ERROR, "프로필 생성 중 오류 발생");
    }
}

// ================================================================
//  onLoginSuccess
//  로그인 성공 후 고객 프로필 조회 → 클라이언트에 전송
//  응답: { status, token, user_id, name, address, point }
// ================================================================
void CustomerHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
    try {
        auto& db = MariaDBManager::getInstance();
        auto rows = db.executeQuery(
            "SELECT u.name, u.address, COALESCE(cp.point, 0) AS point "
            "FROM users u "
            "LEFT JOIN customer_profiles cp ON cp.user_id = u.user_id "
            "WHERE u.user_id=" + std::to_string(userId));

        if (!rows.empty()) {
            json res;
            res["status"]   = Status::SUCCESS;
            res["token"]    = "ctkn_" + std::to_string(userId);
            res["user_id"]  = userId;
            res["login_id"] = reqBody.value("id", "");   // 클라이언트가 "login_id" 키로 수신
            res["name"]     = rows[0].at("name");
            res["address"]  = rows[0].count("address") ? rows[0].at("address") : "";
            res["point"]    = rows[0].count("point") && !rows[0].at("point").empty()
                                ? std::stoi(rows[0].at("point")) : 0;

            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCommon::REQ_LOGIN, res.dump());
        } else {
            sendError(session, CmdCommon::REQ_LOGIN,
                      Status::NOT_FOUND, "프로필 정보를 찾을 수 없습니다.");
        }
    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_LOGIN,
                  Status::SERVER_ERROR, "로그인 처리 중 서버 오류 발생");
    }
}

// ================================================================
//  onLogout
// ================================================================
void CustomerHandler::onLogout(Session* session, int /*userId*/) {
    json res; res["status"] = Status::SUCCESS;
    session->sendPacket(static_cast<uint8_t>(m_clientType),
                        CmdCommon::REQ_LOGOUT, res.dump());
}

// ================================================================
//  onGetProfile
//  기본 프로필 조회 (카드 관련은 Basehandler::handleGetProfile 에서 처리)
// ================================================================
void CustomerHandler::onGetProfile(Session* session, int userId, const json& req) {
    try {
        auto& db = MariaDBManager::getInstance();

        // 주소 변경 요청
        if (req.contains("address")) {
            db.executeUpdate(
                "UPDATE users SET address='" +
                escapeStr(req.value("address", "")) +
                "' WHERE user_id=" + std::to_string(userId));
        }

        // 비밀번호 변경 요청
        if (req.contains("new_pw")) {
            std::string old_pw = req.value("old_pw", "");
            auto chk = db.executeQuery(
                "SELECT user_id FROM users WHERE user_id=" +
                std::to_string(userId) +
                " AND password='" + escapeStr(old_pw) + "'");
            if (chk.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE,
                          Status::UNAUTHORIZED, "현재 비밀번호 불일치");
                return;
            }
            db.executeUpdate(
                "UPDATE users SET password='" +
                escapeStr(req.value("new_pw", "")) +
                "' WHERE user_id=" + std::to_string(userId));
        }

        auto rows = db.executeQuery(
            "SELECT u.name, u.address, u.phone, "
            "COALESCE(cp.point, 0) AS point "
            "FROM users u "
            "LEFT JOIN customer_profiles cp ON cp.user_id = u.user_id "
            "WHERE u.user_id=" + std::to_string(userId));

        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_GET_PROFILE,
                      Status::NOT_FOUND, "사용자 없음");
            return;
        }

        auto& r = rows[0];
        json res;
        res["status"]  = Status::SUCCESS;
        res["name"]    = r.at("name");
        res["address"] = r.count("address") ? r.at("address") : "";
        res["phone"]   = r.count("phone")   ? r.at("phone")   : "";
        res["point"]   = r.count("point") && !r.at("point").empty()
                           ? std::stoi(r.at("point")) : 0;

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCommon::REQ_GET_PROFILE, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_GET_PROFILE,
                  Status::SERVER_ERROR, e.what());
    }
}

// ================================================================
//  handleWithdraw (105)
// ================================================================
void CustomerHandler::handleWithdraw(Session* session, const std::string& /*body*/) {
    int uid = getUserIdByFd(session->getFd());
    if (uid <= 0) {
        sendError(session, CmdCommon::REQ_WITHDRAW,
                  Status::UNAUTHORIZED, "로그인이 필요합니다.");
        return;
    }

    MariaDBManager::getInstance().executeUpdate(
        "UPDATE users SET status='DELETED' WHERE user_id=" +
        std::to_string(uid));
    unregisterSession(session->getFd());

    json res; res["status"] = Status::SUCCESS;
    session->sendPacket(static_cast<uint8_t>(m_clientType),
                        CmdCommon::REQ_WITHDRAW, res.dump());
}
