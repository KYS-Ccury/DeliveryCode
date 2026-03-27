#include "Basehandler.h"
#include "MiddleHandler.h" // MariaDBManager 대신 MiddleHandler 호출!
#include <iostream>

using json = nlohmann::json;

BaseHandler::BaseHandler(ClientType type, const std::string& roleName) 
    : m_clientType(type), m_roleName(roleName) {}

// --- [세션 관리 및 유틸리티는 기존과 동일하게 유지] ---
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
void BaseHandler::sendError(Session* session, uint16_t protocol, uint16_t statusCode, const std::string& message) {
    json res; res["status"] = statusCode; res["message"] = message;
    if (session) session->sendPacket(static_cast<uint8_t>(m_clientType), protocol, res.dump());
}
void BaseHandler::sendResponse(Session* session, uint16_t protocol, const nlohmann::json& payload) {
    if (session) session->sendPacket(static_cast<uint8_t>(m_clientType), protocol, payload.dump());
}

// ================================================================
//  handleLogin (리팩토링 완료)
// ================================================================
void BaseHandler::handleLogin(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        req["role"] = m_roleName; // 역할을 데이터에 추가해서 넘김

        // DB 작업은 MiddleHandler로 완벽하게 위임!
        json dbRes = MiddleHandler::processDBRequest(CmdDBCommon::REQ_DB_LOGIN, req);

        if (dbRes["status"] == Status::SUCCESS) {
            int userId = dbRes["user_id"];
            registerSession(session->getFd(), userId);
            onLoginSuccess(session, userId, req); // 자식 훅 호출
        } else {
            sendError(session, CmdCommon::REQ_LOGIN, dbRes["status"], dbRes.value("message", "로그인 실패"));
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] 로그인 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleSignup (리팩토링 완료)
// ================================================================
void BaseHandler::handleSignup(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        req["role"] = m_roleName;

        // ID 중복 체크든, 실제 회원가입이든 몽땅 MiddleHandler로 넘김
        json dbRes = MiddleHandler::processDBRequest(CmdDBCommon::REQ_DB_SIGNUP, req);

        if (dbRes["status"] == Status::SUCCESS) {
            if (req.value("action", "") == "CHECK_ID") {
                sendResponse(session, CmdCommon::REQ_SIGNUP, dbRes);
                return;
            }
            int userId = dbRes["user_id"];
            // DB에서 발급받은 userId를 자식에게 넘겨줌!
            onSignup(session, userId, req); 
        } else {
            sendError(session, CmdCommon::REQ_SIGNUP, dbRes["status"], dbRes.value("message", "가입 실패"));
        }
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
//  handleGetProfile (리팩토링 완료)
// ================================================================
void BaseHandler::handleGetProfile(Session* session, const std::string& jsonBody) {
    int userId = getUserIdByFd(session->getFd());
    if (userId == -1) {
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "로그인이 필요합니다.");
        return;
    }
    try {
        json req = json::parse(jsonBody.empty() ? "{}" : jsonBody);
        req["user_id"] = userId;

        // 복잡한 쿼리는 전부 CommonDB로 넘겨버림
        json dbRes = MiddleHandler::processDBRequest(CmdDBCommon::REQ_DB_GET_PROFILE, req);

        if (dbRes["status"] == Status::SUCCESS) {
            if (dbRes.contains("is_profile_basic")) {
                // 카드 관련 조회가 아니라면 자식 훅으로 위임
                onGetProfile(session, userId, req);
            } else {
                sendResponse(session, CmdCommon::REQ_GET_PROFILE, dbRes);
            }
        } else {
            sendError(session, CmdCommon::REQ_GET_PROFILE, dbRes["status"], dbRes.value("message", "요청 실패"));
        }
    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] getProfile 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, "서버 오류");
    }
}