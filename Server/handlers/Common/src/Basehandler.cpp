#include "Basehandler.h"
#include "MariaDBManager.h"
#include <iostream>

using json = nlohmann::json;

BaseHandler::BaseHandler(ClientType type, const std::string& roleName) 
    : m_clientType(type), m_roleName(roleName) {}

// --- [세션 관리 구현부] ---

void BaseHandler::registerSession(int fd, int userId) {
    std::lock_guard<std::mutex> lock(m_sessionMtx);
    m_fdToUserId[fd] = userId;
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

// --- [유틸리티 및 공통 로직] ---

void BaseHandler::sendError(Session* session, uint16_t protocol, uint16_t statusCode, const std::string& message) {
    json res; 
    res["status"] = statusCode; 
    res["message"] = message;
    
    if (session) {
        // m_clientType 멤버 변수를 사용하도록 수정 (RIDER 고정 해제)
        session->sendPacket(static_cast<uint8_t>(m_clientType), protocol, res.dump());
    }
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

void BaseHandler::sendResponse(Session* session, uint16_t protocol, const nlohmann::json& payload) {
    if (session) {
        session->sendPacket(static_cast<uint8_t>(m_clientType), protocol, payload.dump());
    }
}

// --- [공통 핸들러 로직] ---

void BaseHandler::handleLogin(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string loginId = req.value("login_id", "");
        std::string password = req.value("password", "");

        if (loginId.empty() || password.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::BAD_REQUEST, "아이디/비밀번호를 입력하세요.");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        std::string q = "SELECT user_id FROM users WHERE login_id = '" + escapeStr(loginId) + 
                        "' AND password = '" + escapeStr(password) + 
                        "' AND role = '" + m_roleName + "' AND status = 'ACTIVE' LIMIT 1";

        DBResult rows = db.executeQuery(q);
        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "정보가 올바르지 않거나 권한이 없습니다.");
            return;
        }

        int userId = std::stoi(rows[0].at("user_id"));
        registerSession(session->getFd(), userId);
        
        // 자식 클래스에서 구현한 훅 호출
        onLoginSuccess(session, userId, req);

    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] 로그인 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, "서버 오류");
    }
}

// ★ 누락되었던 handleSignup, handleLogout, handleGetProfile의 기본 구현 추가
void BaseHandler::handleSignup(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        onSignup(session, req); // 자식에게 위임
    } catch (...) { sendError(session, 0, Status::BAD_REQUEST, "JSON 파싱 에러"); }
}

void BaseHandler::handleLogout(Session* session, const std::string& jsonBody) {
    int userId = getUserIdByFd(session->getFd());
    if (userId != -1) {
        onLogout(session, userId);
        unregisterSession(session->getFd());
    }
}

void BaseHandler::handleGetProfile(Session* session, const std::string& jsonBody) {
    int userId = getUserIdByFd(session->getFd());
    if (userId == -1) {
        sendError(session, 0, Status::UNAUTHORIZED, "로그인이 필요합니다.");
        return;
    }
    try {
        onGetProfile(session, userId, json::parse(jsonBody));
    } catch (...) { /* 에러 처리 */ }
}