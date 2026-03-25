#pragma once
#include <stdint.h>

// ============================================================
//  1. 클라이언트 타입 식별자 (1바이트 헤더용)
// ============================================================
enum class ClientType : uint8_t {
    CUSTOMER = 1,
    OWNER    = 2,
    RIDER    = 3,
    ADMIN    = 4
};

// ============================================================
//  2. 서버 내부 에러 코드 (내부 로직 예외 처리용)
// ============================================================
namespace InternalError {
    constexpr int JSON_PARSE_FAILED = -1;
    constexpr int DB_QUERY_FAILED   = -2;
    constexpr int SOCKET_SEND_ERROR = -3;
}

// ============================================================
//  3. 네트워크 상태 코드 (클라이언트 응답용 Status)
// ============================================================
namespace Status {
    constexpr uint16_t SUCCESS      = 2000; // 정상 처리
    constexpr uint16_t BAD_REQUEST  = 4000; // 파라미터 누락/오류
    constexpr uint16_t UNAUTHORIZED = 4001; // 유효하지 않은 토큰/로그인
    constexpr uint16_t FORBIDDEN    = 4003; // 권한 부족
    constexpr uint16_t NOT_FOUND    = 4004; // 데이터 없음
    constexpr uint16_t SERVER_ERROR = 5000; // 서버 내부 오류
}

// ============================================================
//  4. 기능별 프로토콜 (Command IDs) - 2바이트 헤더용
// ============================================================

// [100번대] 공통 / 인증 프로토콜
namespace CmdCommon {
    constexpr uint16_t REQ_SIGNUP        = 100; // 회원가입
    constexpr uint16_t REQ_LOGIN         = 101; // 로그인
    constexpr uint16_t REQ_LOGOUT        = 102; // 로그아웃
    constexpr uint16_t REQ_TOKEN_REFRESH = 103; // 토큰 갱신
    constexpr uint16_t REQ_GET_PROFILE   = 104; // 정보 조회
    constexpr uint16_t REQ_WITHDRAW      = 105; // 회원 탈퇴
}

// [200번대] 고객용 (Customer)
namespace CmdCustomer {
    
    constexpr uint16_t REQ_STORE_LIST    = 200; // 매장 목록 조회
    constexpr uint16_t REQ_MENU_LIST     = 201; // 메뉴 조회
    constexpr uint16_t REQ_CREATE_ORDER  = 202; // 주문 생성
    constexpr uint16_t REQ_ORDER_HISTORY = 203; // 주문 내역 조회
    constexpr uint16_t REQ_ORDER_DETAIL  = 204; // 주문 상세 조회
    constexpr uint16_t REQ_PAYMENT       = 205; // 결제 요청
    constexpr uint16_t REQ_WRITE_REVIEW  = 206; // 리뷰 작성
    constexpr uint16_t REQ_REVIEW_LIST   = 207; // 리뷰 목록 조회
    constexpr uint16_t REQ_CANCEL_ORDER  = 208; // 주문 취소
    constexpr uint16_t REQ_MY_POINT      = 209; // 포인트조회

    constexpr uint16_t NTF_ORDER_STATUS  = 210; // [알림] 주문 상태 변경 알림
    constexpr uint16_t REQ_CHANGE_PASSWORD = 211; // 개인정보 변경
    constexpr uint16_t REQ_MY_INFO       = 212; // 개인정보조회
}

// [300번대] 사장님용 (Owner)
namespace CmdOwner {
    constexpr uint16_t REQ_STORE_INFO    = 300; // 매장 정보 조회
    constexpr uint16_t REQ_UPDATE_STORE  = 301; // 매장 정보 수정
    constexpr uint16_t REQ_ADD_MENU      = 302; // 메뉴 등록
    constexpr uint16_t REQ_UPDATE_MENU   = 303; // 메뉴 수정
    constexpr uint16_t REQ_ORDER_LIST    = 304; // 주문 목록 조회
    constexpr uint16_t REQ_ACCEPT_ORDER  = 305; // 주문 수락
    constexpr uint16_t REQ_REJECT_ORDER  = 306; // 주문 거절
    constexpr uint16_t REQ_COOKING_DONE  = 307; // 조리 완료 처리
    constexpr uint16_t REQ_SALES_STATS   = 308; // 매출 통계 조회
    constexpr uint16_t REQ_CHANGE_STATUS = 309; // 영업 상태 변경
    
    constexpr uint16_t NTF_NEW_ORDER     = 310; // [알림] 새 주문 알림
    constexpr uint16_t NTF_RIDER_MATCHED = 311; // [알림] 배차 정보 알림
}

// [400번대] 라이더용 (Rider)
namespace CmdRider {
    constexpr uint16_t REQ_DISPATCH_LIST   = 400; // 배차 리스트 조회
    constexpr uint16_t REQ_ACCEPT_DISPATCH = 401; // 배차 수락
    constexpr uint16_t REQ_REJECT_DISPATCH = 402; // 배차 거절
    constexpr uint16_t REQ_PICKUP_DONE     = 403; // 픽업 완료
    constexpr uint16_t REQ_DELIVERY_DONE   = 404; // 배달 완료
    constexpr uint16_t REQ_MY_DISPATCHES   = 405; // 내 배차 목록
    constexpr uint16_t REQ_WORK_STATUS     = 406; // 출퇴근 설정
    constexpr uint16_t REQ_SEND_GPS        = 407; // 위치 전송(GPS)
    
    constexpr uint16_t NTF_NEW_DISPATCH    = 408; // [알림] 신규 주문 배차 알림
}

// [500번대] 관리자용 (Admin)
namespace CmdAdmin {
    constexpr uint16_t REQ_SETTLEMENT_LIST = 500; // 정산 목록 조회
    constexpr uint16_t REQ_SETTLEMENT_DET  = 501; // 정산 상세 조회
    constexpr uint16_t REQ_SETTLEMENT_CONF = 502; // 정산 확정
    constexpr uint16_t REQ_MONITOR_ORDERS  = 510; // 대기 주문 모니터링
    constexpr uint16_t REQ_RIDER_STATUS    = 511; // 라이더 현황
    constexpr uint16_t REQ_FORCE_DISPATCH  = 512; // 강제 배차
    constexpr uint16_t REQ_FORCE_CANCEL    = 513; // 배차 강제 취소
    constexpr uint16_t REQ_MANAGE_REVIEW   = 520; // 리뷰 관리
}

// [600번대] 채팅 공통 (Chat)
namespace CmdChat {
    constexpr uint16_t REQ_CREATE_ROOM     = 600; // 채팅방 생성
    constexpr uint16_t REQ_SEND_MSG        = 601; // 메시지 전송
    constexpr uint16_t REQ_GET_MSGS        = 602; // 메시지 조회
    constexpr uint16_t REQ_ROOM_LIST       = 603; // 채팅방 목록
    constexpr uint16_t REQ_READ_RECEIPT    = 605; // 읽음 확인
    
    constexpr uint16_t NTF_RECV_MSG        = 604; // [알림] 메시지 수신
}