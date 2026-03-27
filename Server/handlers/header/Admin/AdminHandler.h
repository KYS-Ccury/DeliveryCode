// #pragma once

#include "Basehandler.h"
#include "Admin_Auth.h"
#include "Admin_UserManage.h"
#include "Admin_Chat.h"

// ============================================================
// AdminHandler — 관리자 패킷 라우터 (싱글톤)
// Dispatcher에서 clientType=ADMIN 패킷을 받아 기능별로 분기한다.
// 실제 처리는 Admin_Auth, Admin_UserManage, Admin_Chat에 위임한다.
// ============================================================
class AdminHandler : public BaseHandler {
public:
    // 싱글톤 인스턴스를 반환한다.
    static AdminHandler& getInstance();

    // BaseHandler 순수 가상함수 override (4개)
    void onSignup      (Session* session, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout      (Session* session, int userId) override;
    void onGetProfile  (Session* session, int userId, const nlohmann::json& reqBody) override;

    // Dispatcher에서 호출하는 메인 진입점 — 프로토콜 번호로 분기한다.
    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

private:
    // 생성자 (싱글톤이므로 private)
    AdminHandler();

    // 기능별 처리 객체
    AdminAuth        m_auth;
    AdminUserManage  m_userManage;
    AdminChat        m_chat;
};