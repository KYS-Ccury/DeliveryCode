// #pragma once

// #include "Session.h"
// #include <string>

// class BaseHandler;

// // ============================================================
// // AdminUserManage — 관리자 전용 관리 기능
// // 주문 모니터링(510), 라이더 현황(511),
// // 강제 배차(512), 배차 취소(513), 리뷰 관리(520)를 처리한다.
// // ============================================================
// class AdminUserManage {
// public:
//     // 생성자 — 부모 핸들러 참조를 받는다.
//     explicit AdminUserManage(BaseHandler& handler);

//     // 대기 주문 목록을 조회한다. (510)
//     void handleOrderMonitor (Session* session, const std::string& jsonBody);

//     // 라이더 현황을 조회한다. (511)
//     void handleRiderStatus  (Session* session, const std::string& jsonBody);

//     // 강제 배차를 수행한다. (512)
//     void handleForceDispatch(Session* session, const std::string& jsonBody);

//     // 배차를 강제 취소한다. (513)
//     void handleForceCancel  (Session* session, const std::string& jsonBody);

//     // 리뷰를 관리한다 (목록/삭제/숨김). (520)
//     void handleManageReview (Session* session, const std::string& jsonBody);

// private:
//     // 부모 핸들러 참조 (sendResponse, sendError 호출용)
//     BaseHandler& m_handler;
// };