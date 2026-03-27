#include "AdminHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;


AdminAuth::AdminAuth(BaseHandler& handler) : m_handler(handler) {}

// ============================================================
// adminLoginSuccess — 관리자 로그인 성공 후처리를 수행한다.
// BaseHandler::handleLogin()이 DB 조회 후 성공 시 호출한다.
// 세션에 userId와 userType을 세팅하고 성공 응답을 보낸다.
// ============================================================
void AdminHandler::adminLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 관리자 로그인 성공 (userId=" << userId << ", fd=" << session->getFd() << ")" << std::endl;
    std::cout << "-------------------------" << std::endl;

    // 세션에 유저 정보를 저장한다.
    session->setUserID(userId);
    session->setUserType(static_cast<uint8_t>(ClientType::ADMIN));

    // 성공 응답을 전송한다.
    json res;
    res["status"]  = Status::SUCCESS;
    res["message"] = "관리자 로그인 성공";
    res["user_id"] = userId;
    sendResponse(session, CmdCommon::REQ_LOGIN, res);
}

// ============================================================
// adminLogout — 관리자 로그아웃 후처리를 수행한다.
// ============================================================
void AdminHandler::adminLogout(Session* session, int userId)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 관리자 로그아웃 (userId=" << userId << ")" << std::endl;
    std::cout << "-------------------------" << std::endl;

    // 로그아웃 성공 응답을 전송한다.
    json res;
    res["status"]  = Status::SUCCESS;
    res["message"] = "로그아웃 완료";
    sendResponse(session, CmdCommon::REQ_LOGOUT, res);
}

// ============================================================
// adminGetProfile — 관리자 프로필을 조회한다.
// DB에서 user_id로 검색하여 login_id, role을 반환한다.
// ============================================================
void AdminHandler::adminGetProfile(Session* session, int userId, const nlohmann::json& reqBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 프로필 조회 (userId=" << userId << ")" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        auto& db = MariaDBManager::getInstance();

        // DB에서 관리자 정보를 조회한다.
        std::string q = "SELECT user_id, login_id, role, status FROM users WHERE user_id = "
                        + std::to_string(userId) + " LIMIT 1";
        DBResult rows = db.executeQuery(q);

        if (rows.empty()) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 프로필 조회 실패 (유저 없음, userId=" << userId << ")" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdCommon::REQ_GET_PROFILE, Status::NOT_FOUND, "사용자 없음");
            return;
        }

        // 조회 결과를 응답으로 전송한다.
        json res;
        res["status"]   = Status::SUCCESS;
        res["user_id"]  = rows[0].count("user_id")  ? rows[0].at("user_id")  : "";
        res["login_id"] = rows[0].count("login_id") ? rows[0].at("login_id") : "";
        res["role"]     = rows[0].count("role")      ? rows[0].at("role")      : "";
        sendResponse(session, CmdCommon::REQ_GET_PROFILE, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 프로필 조회 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, "서버 오류");
    }
}