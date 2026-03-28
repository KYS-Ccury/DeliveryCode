/**
 * AdminHandler.cpp
 * ============================================================
 * ★ 수정사항:
 *   1) 600번대 채팅 switch 주석 해제
 *   2) 하트비트(200/201) 응답 핸들러 추가
 * ============================================================
 */

#include "AdminHandler.h"
#include "Protocol.h"
#include <iostream>

// ============================================================
// 싱글톤 생성자
// ============================================================
AdminHandler::AdminHandler()
    : BaseHandler(ClientType::ADMIN, "ADMIN")
    , m_auth(*this)
    , m_userManage(*this)
    , m_chat(*this)
{
}

// ============================================================
// 싱글톤 인스턴스를 반환한다.
// ============================================================
AdminHandler& AdminHandler::getInstance()
{
    static AdminHandler instance;
    return instance;
}

// ============================================================
// process() — Dispatcher에서 호출하는 메인 진입점이다.
// 프로토콜 번호에 따라 인증/관리/채팅 함수로 분기한다.
// ============================================================
void AdminHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : process() protocol=" << protocol << std::endl;
    std::cout << "-------------------------" << std::endl;

    // ── 100번대: 공통 프로토콜 → BaseHandler 공통 처리 ──
    switch (protocol)
    {
    case CmdCommon::REQ_LOGIN:       handleLogin(session, jsonBody);      return;
    case CmdCommon::REQ_LOGOUT:      handleLogout(session, jsonBody);     return;
    case CmdCommon::REQ_SIGNUP:      handleSignup(session, jsonBody);     return;
    case CmdCommon::REQ_GET_PROFILE: handleGetProfile(session, jsonBody); return;
    }

    // ── ★ 하트비트 응답 (클라이언트 폴링용) ──
    if (protocol == 200)
    {
        handleHeartbeat(session, jsonBody);
        return;
    }

    // ── 500번대: 관리자 전용 기능 ──
    switch (protocol)
    {
    case CmdAdmin::REQ_MONITOR_ORDERS: handleOrderMonitor (session, jsonBody); return;
    case CmdAdmin::REQ_RIDER_STATUS:   handleRiderStatus  (session, jsonBody); return;
    case CmdAdmin::REQ_FORCE_DISPATCH: handleForceDispatch(session, jsonBody); return;
    case CmdAdmin::REQ_FORCE_CANCEL:   handleForceCancel  (session, jsonBody); return;
    case CmdAdmin::REQ_MANAGE_REVIEW:  handleManageReview (session, jsonBody); return;
    }

    // ── ★ 600번대: 채팅 (주석 해제) ──
    switch (protocol)
    {
    case CmdChat::REQ_SEND_MSG:   handleSendMsg (session, jsonBody); return;
    case CmdChat::REQ_GET_MSGS:   handleGetMsgs (session, jsonBody); return;
    case CmdChat::REQ_ROOM_LIST:  handleRoomList(session, jsonBody); return;
    }

    // 미처리 프로토콜 로그를 출력한다.
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "오류 : 미처리 프로토콜 " << protocol << std::endl;
    std::cout << "-------------------------" << std::endl;
    sendError(session, protocol, Status::BAD_REQUEST, "알 수 없는 요청");
}

// ============================================================
// ★ handleHeartbeat — 클라이언트 폴링 하트비트에 응답한다.
//    요청: protocol=200, body={}
//    응답: protocol=201, body={"status":2000}
// ============================================================
void AdminHandler::handleHeartbeat(Session* session, const std::string& jsonBody)
{
    nlohmann::json res;
    res["status"] = Status::SUCCESS;
    sendResponse(session, 201, res);
}

// ============================================================
// onSignup — 관리자 회원가입은 지원하지 않는다.
// ============================================================
void AdminHandler::onSignup(Session* session, int userId, const nlohmann::json& reqBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "오류 : 회원가입 시도 (미지원)" << std::endl;
    std::cout << "-------------------------" << std::endl;
    sendError(session, CmdCommon::REQ_SIGNUP, Status::FORBIDDEN, "관리자 회원가입은 지원하지 않습니다.");
}

// ============================================================
// onLoginSuccess — Admin_Auth.cpp에 위임한다.
// ============================================================
void AdminHandler::onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody)
{
    adminLoginSuccess(session, userId, reqBody);
}

// ============================================================
// onLogout — Admin_Auth.cpp에 위임한다.
// ============================================================
void AdminHandler::onLogout(Session* session, int userId)
{
    adminLogout(session, userId);
}

// ============================================================
// onGetProfile — Admin_Auth.cpp에 위임한다.
// ============================================================
void AdminHandler::onGetProfile(Session* session, int userId, const nlohmann::json& reqBody)
{
    adminGetProfile(session, userId, reqBody);
}