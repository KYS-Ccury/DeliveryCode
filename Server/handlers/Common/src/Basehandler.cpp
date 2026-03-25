#include "Basehandler.h"
#include "MariaDBManager.h"
#include <iostream>

using json = nlohmann::json;

BaseHandler::BaseHandler(ClientType type, const std::string& roleName) 
    : m_clientType(type), m_roleName(roleName) {}

// --- [세션 관리] ---

void BaseHandler::registerSession(int fd, int userId) {
    std::lock_guard<std::mutex> lock(m_sessionMtx);
    m_fdToUserId[fd]     = userId;
    m_userIdToFd[userId] = fd;
}

void BaseHandler::unregisterSession(int fd) {
    std::lock_guard<std::mutex> lock(m_sessionMtx);
    auto it = m_fdToUserId.find(fd);
    if (it != m_fdToUserId.end()) {
        m_userIdToFd.erase(it->second);
        m_fdToUserId.erase(it);
    }
}

int BaseHandler::getUserIdByFd(int fd) {
    std::lock_guard<std::mutex> lock(m_sessionMtx);
    auto it = m_fdToUserId.find(fd);
    return (it != m_fdToUserId.end()) ? it->second : -1;
}

int BaseHandler::getFdByUserId(int userId) {
    std::lock_guard<std::mutex> lock(m_sessionMtx);
    auto it = m_userIdToFd.find(userId);
    return (it != m_userIdToFd.end()) ? it->second : -1;
}

// --- [유틸리티] ---

void BaseHandler::sendError(Session* session, uint16_t protocol,
                             uint16_t statusCode, const std::string& message) {
    json res;
    res["status"]  = statusCode;
    res["message"] = message;
    if (session)
        session->sendPacket(static_cast<uint8_t>(m_clientType), protocol, res.dump());
}

std::string BaseHandler::escapeStr(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

void BaseHandler::sendResponse(Session* session, uint16_t protocol,
                                const nlohmann::json& payload) {
    if (session)
        session->sendPacket(static_cast<uint8_t>(m_clientType), protocol, payload.dump());
}

// --- [공통 핸들러] ---

// ================================================================
//  handleLogin
//  클라이언트 전송 형식: { "id":"...", "pw":"..." }
//  (기존 "login_id"/"password" → "id"/"pw" 로 수정)
// ================================================================
void BaseHandler::handleLogin(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);

        // 클라이언트가 "id"/"pw" 로 전송하므로 해당 키로 파싱
        std::string loginId  = req.value("id", "");
        std::string password = req.value("pw", "");

        if (loginId.empty() || password.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN,
                      Status::BAD_REQUEST, "아이디/비밀번호를 입력하세요.");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        std::string q =
            "SELECT user_id FROM users WHERE login_id = '" + escapeStr(loginId) +
            "' AND password = '" + escapeStr(password) +
            "' AND role = '" + m_roleName +
            "' AND status = 'ACTIVE' LIMIT 1";

        DBResult rows = db.executeQuery(q);
        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN,
                      Status::UNAUTHORIZED, "아이디 또는 비밀번호가 올바르지 않습니다.");
            return;
        }

        int userId = std::stoi(rows[0].at("user_id"));
        registerSession(session->getFd(), userId);

        // 자식 클래스 훅 호출 (프로필 조회 후 응답 전송)
        onLoginSuccess(session, userId, req);

    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] 로그인 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleSignup
//  클라이언트 전송 형식:
//    { "id":"...", "pw":"...", "name":"...", "phone":"...",
//      "address":"...", "role":1 }
//
//  처리 순서:
//    1. users 테이블 INSERT (공통)
//    2. onSignup 훅 호출 → 자식에서 role별 부가 테이블 INSERT
// ================================================================
void BaseHandler::handleSignup(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);

        // ── 아이디 중복확인 요청 분기 ────────────────────────
        // 클라이언트: { "action":"CHECK_ID", "id":"..." }
        std::string action = req.value("action", "");
        if (action == "CHECK_ID") {
            std::string checkId = escapeStr(req.value("id", ""));
            if (checkId.empty()) {
                sendError(session, CmdCommon::REQ_SIGNUP,
                          Status::BAD_REQUEST, "아이디를 입력하세요.");
                return;
            }
            auto& db = MariaDBManager::getInstance();
            auto dup = db.executeQuery(
                "SELECT user_id FROM users WHERE login_id='" + checkId + "' LIMIT 1");
            json res;
            res["status"]    = Status::SUCCESS;
            res["available"] = dup.empty();  // true = 사용 가능
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCommon::REQ_SIGNUP, res.dump());
            return;
        }

        std::string loginId = escapeStr(req.value("id",      ""));
        std::string pw      = escapeStr(req.value("pw",      ""));
        std::string name    = escapeStr(req.value("name",    ""));
        std::string phone   = escapeStr(req.value("phone",   ""));
        std::string address = escapeStr(req.value("address", ""));

        if (loginId.empty() || pw.empty()) {
            sendError(session, CmdCommon::REQ_SIGNUP,
                      Status::BAD_REQUEST, "아이디/비밀번호를 입력하세요.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 아이디 중복 확인
        auto dup = db.executeQuery(
            "SELECT user_id FROM users WHERE login_id='" + loginId + "' LIMIT 1");
        if (!dup.empty()) {
            sendError(session, CmdCommon::REQ_SIGNUP,
                      Status::BAD_REQUEST, "이미 사용 중인 아이디입니다.");
            return;
        }

        // users INSERT
        bool ok = db.executeUpdate(
            "INSERT INTO users (login_id, password, role, name, phone, address, status) "
            "VALUES ('" + loginId  + "','" + pw      + "','" +
                          m_roleName + "','" + name    + "','" +
                          phone   + "','" + address + "','ACTIVE')");

        if (!ok) {
            sendError(session, CmdCommon::REQ_SIGNUP,
                      Status::SERVER_ERROR, "회원가입 처리 중 오류 발생");
            return;
        }

        // 자식 클래스 훅 호출 (customer_profiles 등 role별 부가 테이블)
        onSignup(session, req);

    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] 회원가입 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "서버 오류");
    }
}

void BaseHandler::handleLogout(Session* session, const std::string& /*jsonBody*/) {
    int userId = getUserIdByFd(session->getFd());
    if (userId != -1) {
        onLogout(session, userId);
        unregisterSession(session->getFd());
    }
}

// ================================================================
//  handleGetProfile
//  request_type 에 따라 분기:
//    "get_cards"      → 카드 목록 반환
//    "add_card"       → 카드 등록
//    "delete_card"    → 카드 삭제
//    "set_default_card" → 기본 카드 설정
//    (없거나 기타)   → 프로필 기본 조회 → onGetProfile 위임
// ================================================================
void BaseHandler::handleGetProfile(Session* session, const std::string& jsonBody) {
    int userId = getUserIdByFd(session->getFd());
    if (userId == -1) {
        sendError(session, CmdCommon::REQ_GET_PROFILE,
                  Status::UNAUTHORIZED, "로그인이 필요합니다.");
        return;
    }
    try {
        json req = json::parse(jsonBody.empty() ? "{}" : jsonBody);
        std::string reqType = req.value("request_type", "");
        auto& db = MariaDBManager::getInstance();

        // ── 카드 목록 조회 ────────────────────────────────────
        if (reqType == "get_cards") {
            auto rows = db.executeQuery(
                "SELECT payment_method_id AS id, card_alias AS alias, "
                "card_num_masked AS masked, method_type, is_default "
                "FROM payment_methods WHERE user_id=" + std::to_string(userId) +
                " ORDER BY is_default DESC, payment_method_id ASC");

            json methods = json::array();
            for (auto& r : rows) {
                json m;
                m["id"]          = std::stoi(r.at("id"));
                m["alias"]       = r.count("alias")   ? r.at("alias")   : "";
                m["masked"]      = r.count("masked")  ? r.at("masked")  : "";
                m["method_type"] = r.count("method_type") ? r.at("method_type") : "CARD";
                m["is_default"]  = (r.count("is_default") && r.at("is_default") == "1");
                methods.push_back(m);
            }
            json res;
            res["status"]           = Status::SUCCESS;
            res["payment_methods"]  = methods;
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 카드 등록 ────────────────────────────────────────
        if (reqType == "add_card") {
            std::string alias   = escapeStr(req.value("card_alias",      "내 카드"));
            std::string masked  = escapeStr(req.value("card_num_masked", ""));
            std::string mtype   = escapeStr(req.value("method_type",     "CARD"));

            db.executeUpdate(
                "INSERT INTO payment_methods "
                "(user_id, method_type, card_alias, card_num_masked, is_default) "
                "VALUES (" + std::to_string(userId) + ",'" +
                mtype + "','" + alias + "','" + masked + "',0)");

            json res; res["status"] = Status::SUCCESS;
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 카드 삭제 ────────────────────────────────────────
        if (reqType == "delete_card") {
            int pmId = req.value("payment_method_id", 0);
            db.executeUpdate(
                "DELETE FROM payment_methods WHERE payment_method_id=" +
                std::to_string(pmId) + " AND user_id=" + std::to_string(userId));

            json res; res["status"] = Status::SUCCESS;
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 기본 카드 설정 ────────────────────────────────────
        if (reqType == "set_default_card") {
            int pmId = req.value("payment_method_id", 0);
            // 기존 기본 카드 해제
            db.executeUpdate(
                "UPDATE payment_methods SET is_default=0 WHERE user_id=" +
                std::to_string(userId));
            // 새 기본 카드 설정
            db.executeUpdate(
                "UPDATE payment_methods SET is_default=1 "
                "WHERE payment_method_id=" + std::to_string(pmId) +
                " AND user_id=" + std::to_string(userId));

            json res; res["status"] = Status::SUCCESS;
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 기본: 프로필 조회 → 자식 훅 위임 ─────────────────
        onGetProfile(session, userId, req);

    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] getProfile 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, "서버 오류");
    }
}
