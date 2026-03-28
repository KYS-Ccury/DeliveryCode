#pragma once

#include "Session.h"
#include <string>

class BaseHandler;

// ============================================================
// AdminChat — 관리자 채팅 처리
// 메시지 전송(601), 메시지 조회(602), 채팅방 목록(603)을 처리한다.
// ============================================================
class AdminChat {
public:
    // 생성자 — 부모 핸들러 참조를 받는다.
    explicit AdminChat(BaseHandler& handler);

    // 관리자가 메시지를 전송한다. (601)
    void handleSendMsg (Session* session, const std::string& jsonBody);

    // 채팅 메시지를 조회한다. (602)
    void handleGetMsgs (Session* session, const std::string& jsonBody);

    // 채팅방 목록을 조회한다. (603)
    void handleRoomList(Session* session, const std::string& jsonBody);

private:
    // 부모 핸들러 참조 (sendResponse, sendError, getUserIdByFd, escapeStr 호출용)
    BaseHandler& m_handler;
};