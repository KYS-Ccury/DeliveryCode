#include "Basehandler.h"
#include "MariaDBManager.h"
#include <iostream>

using json = nlohmann::json;

BaseHandler::BaseHandler(ClientType type, const std::string& roleName) 
    : m_clientType(type), m_roleName(roleName) {}

// ... 세션 관리(registerSession 등) 및 헬퍼 함수 구현 생략 (기존과 동일) ...

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
        
        // ★ m_roleName 을 사용하여 한 번의 쿼리로 4가지 권한 모두 검증 가능!
        std::string q = "SELECT user_id FROM users WHERE login_id = '" + escapeStr(loginId) + 
                        "' AND password = '" + escapeStr(password) + 
                        "' AND role = '" + m_roleName + "' AND status = 'ACTIVE' LIMIT 1";

        DBResult rows = db.executeQuery(q);
        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "아이디 또는 비밀번호가 올바르지 않거나 권한이 없습니다.");
            return;
        }

        int userId = std::stoi(rows[0].at("user_id"));

        // 공통 세션 등록
        registerSession(session->getFd(), userId);

        // 자식 클래스(Rider, Customer 등)에 정의된 상세 로직(프로필 조회 등) 실행
        onLoginSuccess(session, userId, req);

    } catch (const std::exception& e) {
        std::cerr << "[" << m_roleName << "] 로그인 오류: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, "서버 오류가 발생했습니다.");
    }
}