// #pragma once

// #include "Session.h"
// #include <nlohmann/json.hpp>
// #include <string>

// class BaseHandler;

// // ============================================================
// // AdminAuth — 관리자 인증 처리 (로그인 성공/로그아웃/프로필 조회)
// // AdminHandler가 소유하며, BaseHandler 훅에서 위임받아 처리한다.
// // ============================================================
// class AdminAuth {
// public:
//     // 생성자 — 부모 핸들러 참조를 받는다.
//     explicit AdminAuth(BaseHandler& handler);

//     // 로그인 성공 후처리를 수행한다.
//     void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody);

//     // 로그아웃 후처리를 수행한다.
//     void onLogout(Session* session, int userId);

//     // 프로필 조회를 수행한다.
//     void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody);

// private:
//     // 부모 핸들러 참조 (sendResponse, sendError 호출용)
//     BaseHandler& m_handler;
// };