/**
 * AdminHandler.h
 * ============================================================
 * ★ 수정사항:
 *   1) handleHeartbeat() 선언 추가
 * ============================================================
 */

#pragma once

#include "Basehandler.h"
#include "Admin_auth.h"
#include "Admin_usermanage.h"
#include "Admin_chat.h"

class AdminHandler : public BaseHandler {
public:
    static AdminHandler& getInstance();

    // BaseHandler 순수 가상함수 override (4개)
    void onSignup      (Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout      (Session* session, int userId) override;
    void onGetProfile  (Session* session, int userId, const nlohmann::json& reqBody) override;

    void adminLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody);
    void adminLogout      (Session* session, int userId);
    void adminGetProfile  (Session* session, int userId, const nlohmann::json& reqBody);

    // Dispatcher에서 호출하는 메인 진입점
    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

    // ── ★ 하트비트 응답 (200 → 201) ─────────────────────────
    void handleHeartbeat(Session* session, const std::string& jsonBody);

    // ── 관리자 전용 기능 (Admin_usermanage.cpp) ───────────────
    void handleOrderMonitor (Session* session, const std::string& jsonBody); // 510
    void handleRiderStatus  (Session* session, const std::string& jsonBody); // 511
    void handleForceDispatch(Session* session, const std::string& jsonBody); // 512
    void handleForceCancel  (Session* session, const std::string& jsonBody); // 513
    void handleManageReview (Session* session, const std::string& jsonBody); // 520

    // ── 채팅 기능 (Admin_chat.cpp) ───────────────────────────
    void handleSendMsg (Session* session, const std::string& jsonBody); // 601
    void handleGetMsgs (Session* session, const std::string& jsonBody); // 602
    void handleRoomList(Session* session, const std::string& jsonBody); // 603

private:
    AdminHandler();

    AdminAuth        m_auth;
    AdminUserManage  m_userManage;
    AdminChat        m_chat;
};