#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mutex>

// ============================================================
// AdminDBHandler — 관리자 전용 DB 처리 (1500번대)
// MiddleHandler에서 호출할 라우팅 전담 진입점을 제공한다.
// 모든 함수는 static이며 DB 쿼리를 수행하고 JSON 결과를 반환한다.
// ============================================================
class AdminDBHandler {
public:
    // MiddleHandler에서 호출할 1500번대 라우팅 전담 진입점
    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);

private:
    // DB 동시접근 보호용 뮤텍스
    static std::mutex admin_db_mutex;

    // [AdminDB.cpp] - 인증 관련 (Auth)
    // 관리자 로그인 DB 조회를 수행한다. (1101 → ADMIN용)
    static nlohmann::json loginHook(const nlohmann::json& reqJson);
    // 관리자 로그아웃 DB 처리를 수행한다.
    static nlohmann::json logoutHook(const nlohmann::json& reqJson);
    // 관리자 프로필 DB 조회를 수행한다.
    static nlohmann::json getProfile(const nlohmann::json& reqJson);

    // [AdminDB_Manage.cpp] - 주문/배차/라이더/리뷰 관리
    // 대기 주문 목록을 DB에서 조회한다. (1510)
    static nlohmann::json monitorOrders(const nlohmann::json& reqJson);
    // 라이더 현황을 DB에서 조회한다. (1511)
    static nlohmann::json riderStatus(const nlohmann::json& reqJson);
    // 강제 배차를 DB에서 수행한다. (1512)
    static nlohmann::json forceDispatch(const nlohmann::json& reqJson);
    // 배차 강제 취소를 DB에서 수행한다. (1513)
    static nlohmann::json forceCancel(const nlohmann::json& reqJson);
    // 리뷰 관리(목록/삭제/숨김)를 DB에서 수행한다. (1520)
    static nlohmann::json manageReview(const nlohmann::json& reqJson);

    // [AdminDB_Chat.cpp] - 채팅 관련
    // 메시지를 DB에 저장한다. (601용)
    static nlohmann::json sendMsg(const nlohmann::json& reqJson);
    // 채팅 메시지를 DB에서 조회한다. (602용)
    static nlohmann::json getMsgs(const nlohmann::json& reqJson);
    // 채팅방 목록을 DB에서 조회한다. (603용)
    static nlohmann::json roomList(const nlohmann::json& reqJson);
};