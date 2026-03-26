// #pragma once

// #include "Basehandler.h"

// class AdminHandler : public BaseHandler {
// public:
//     static AdminHandler& getInstance();

//     // ── BaseHandler 순수 가상함수 override (4개) ──
//     void onSignup      (Session* session, const nlohmann::json& reqBody) override;
//     void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
//     void onLogout      (Session* session, int userId) override;
//     void onGetProfile  (Session* session, int userId, const nlohmann::json& reqBody) override;

//     // ── Dispatcher 진입점 ──
//     void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

// private:
//     AdminHandler();

//     // ── 500번대: 관리자 기능 ──
//     void handleOrderMonitor (Session* session, const std::string& jsonBody);
//     void handleRiderStatus  (Session* session, const std::string& jsonBody);
//     void handleForceDispatch(Session* session, const std::string& jsonBody);
//     void handleForceCancel  (Session* session, const std::string& jsonBody);
//     void handleManageReview (Session* session, const std::string& jsonBody);

//     // ── 600번대: 채팅 ──
//     void handleSendMsg  (Session* session, const std::string& jsonBody);
//     void handleGetMsgs  (Session* session, const std::string& jsonBody);
//     void handleRoomList (Session* session, const std::string& jsonBody);
// };