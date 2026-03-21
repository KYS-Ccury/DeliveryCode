#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include <mutex>

class Session; // 전방 선언

// ============================================================
//  RiderHandler
//  라이더 클라이언트(ClientType::RIDER, 3)의 모든 요청을 처리
//  공통 프로토콜(100번대)도 이 핸들러에서 분기 처리
// ============================================================
class RiderHandler {
public:
    // ─── 메인 디스패치 함수 ───────────────────────────────
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

    // ─── 세션 관리 (fd → riderId 매핑) ──────────────────
    // 로그인 성공 시 등록, 로그아웃/접속종료 시 제거
    static void registerSession(int fd, int riderId);
    static void unregisterSession(int fd);
    static int  getRiderIdByFd(int fd);   // 0이면 미등록(비로그인)

    // ─── 서버 Push: 관리자가 배차 요청을 보낼 때 사용 ──
    // (AdminHandler 또는 주문 수락 시 호출)
    static bool pushDispatch(int riderFd, int orderId,
                             const std::string& storeName,
                             const std::string& pickupAddr,
                             const std::string& destAddr,
                             int deliveryFee);

private:
    // ─── 공통 인증 ─────────────────────────────────────
    static void handleSignup    (Session* session, const std::string& jsonBody); // 100 회원가입
    static void handleLogin     (Session* session, const std::string& jsonBody); // 101 로그인
    static void handleLogout    (Session* session, const std::string& jsonBody); // 102 로그아웃
    static void handleGetProfile(Session* session, const std::string& jsonBody); // 104 프로필/변경

    // ─── 라이더 전용 기능 ──────────────────────────────
    static void handleDispatchList  (Session* session, const std::string& jsonBody); // 400 배차 리스트
    static void handleAcceptDispatch(Session* session, const std::string& jsonBody); // 401 배차 수락
    static void handleRejectDispatch(Session* session, const std::string& jsonBody); // 402 배차 거절
    static void handlePickupDone    (Session* session, const std::string& jsonBody); // 403 픽업 완료
    static void handleDeliveryDone  (Session* session, const std::string& jsonBody); // 404 배달 완료
    static void handleMyDispatches  (Session* session, const std::string& jsonBody); // 405 내 배달 내역
    static void handleWorkStatus    (Session* session, const std::string& jsonBody); // 406 출퇴근/배차수락 상태
    static void handleUpdateGps     (Session* session, const std::string& jsonBody); // 407 GPS 위치

    // ─── 세션 테이블 (fd ↔ riderId) ────────────────────
    // fd: 소켓 파일 디스크립터, riderId: users.user_id
    static std::unordered_map<int, int> s_fdToRider; // fd  → riderId
    static std::unordered_map<int, int> s_riderToFd; // riderId → fd
    static std::mutex                   s_sessionMtx;
};
