#pragma once

#include "Basehandler.h"

/**
 * AdminHandler.h
 * ============================================================
 * 관리자(Admin) 전용 핸들러 — 싱글톤
 *
 * [100번대] 공통 — BaseHandler가 처리 (로그인/로그아웃)
 * [500번대] 관리자 전용
 *   510: 대기 주문 모니터링
 *   511: 라이더 현황
 *   512: 강제 배차
 *   513: 배차 강제 취소
 *   520: 리뷰 관리 (list / delete / toggle_visibility)
 * [600번대] 채팅 (관리자 → 고객 1:1 문의)
 *   601: 메시지 전송
 *   602: 메시지 조회
 *   603: 채팅방 목록
 * ============================================================
 */
class AdminHandler : public BaseHandler {
public:
    // 싱글톤
    static AdminHandler& getInstance();

    // Dispatcher에서 호출하는 메인 분기 함수
    void handlePacket(Session* session, uint16_t protocol, const std::string& jsonBody);

protected:
    // BaseHandler 훅
    void onLoginSuccess(Session* session, int userId, const json& req) override;
    void onLogout      (Session* session, int userId) override;

private:
    AdminHandler();

    // ── 500번대: 관리자 기능 ──
    void handleOrderMonitor (Session* session, const std::string& jsonBody);  // 510
    void handleRiderStatus  (Session* session, const std::string& jsonBody);  // 511
    void handleForceDispatch(Session* session, const std::string& jsonBody);  // 512
    void handleForceCancel  (Session* session, const std::string& jsonBody);  // 513
    void handleManageReview (Session* session, const std::string& jsonBody);  // 520

    // ── 600번대: 채팅 ──
    void handleSendMsg  (Session* session, const std::string& jsonBody);  // 601
    void handleGetMsgs  (Session* session, const std::string& jsonBody);  // 602
    void handleRoomList (Session* session, const std::string& jsonBody);  // 603
};