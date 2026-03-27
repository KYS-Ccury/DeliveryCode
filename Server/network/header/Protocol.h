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
    constexpr uint16_t REQ_CANCEL_ORDER      = 208; // 주문 취소
    constexpr uint16_t REQ_MY_POINT          = 209; // 포인트 조회
    constexpr uint16_t NTF_ORDER_STATUS      = 210; // [알림] 주문 상태 변경 알림
    constexpr uint16_t REQ_CHANGE_PASSWORD   = 211; // 비밀번호 변경
    constexpr uint16_t REQ_MY_INFO           = 212; // 내 정보 조회

    // ★ 주소 관련 프로토콜 (신규)
    constexpr uint16_t REQ_GET_ADDRESSES    = 213; // 주소 목록 조회
    constexpr uint16_t REQ_SAVE_ADDRESS     = 214; // 주소 추가
    constexpr uint16_t REQ_DELETE_ADDRESS   = 215; // 주소 삭제
    constexpr uint16_t REQ_DEFAULT_ADDRESS  = 216; // 기본 주소 설정

    // ★ 이미지 요청 프로토콜 (신규)
    constexpr uint16_t REQ_GET_IMAGE        = 217; // 이미지 파일 요청 (base64 응답)
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
    constexpr uint16_t NTF_RECV_MSG        = 604; // [알림] 메시지 수신
    constexpr uint16_t REQ_READ_RECEIPT    = 605; // 읽음 확인
}

// ============================================================
//  5. DB 처리용 프로토콜 (Command IDs) - 서버 내부 통신
// ============================================================

// [1100번대] 공통 / 인증 DB 처리
namespace CmdDBCommon {
    constexpr uint16_t REQ_DB_SIGNUP        = 1100; // 회원가입 DB Insert
    constexpr uint16_t REQ_DB_LOGIN         = 1101; // 로그인 계정 확인 DB Select
    constexpr uint16_t REQ_DB_GET_PROFILE   = 1104; // 정보 조회 DB Select
}

// [1200번대] 고객 DB 처리 (Customer)
namespace CmdDBCustomer {
    constexpr uint16_t REQ_DB_STORE_LIST    = 1200; // 매장 목록 조회 DB Select
    constexpr uint16_t REQ_DB_MENU_LIST     = 1201; // 메뉴 조회 DB Select
    constexpr uint16_t REQ_DB_CREATE_ORDER  = 1202; // 주문 생성 DB Insert
    constexpr uint16_t REQ_DB_ORDER_HISTORY = 1203; // 주문 내역 조회 DB Select
    constexpr uint16_t REQ_DB_ORDER_DETAIL  = 1204; // 주문 상세 조회 DB Select
    constexpr uint16_t REQ_DB_PAYMENT       = 1205; // 결제 정보 DB Insert/Update
    constexpr uint16_t REQ_DB_WRITE_REVIEW  = 1206; // 리뷰 작성 DB Insert
    constexpr uint16_t REQ_DB_REVIEW_LIST   = 1207; // 리뷰 목록 조회 DB Select
    constexpr uint16_t REQ_DB_CANCEL_ORDER  = 1208; // 주문 취소 상태 DB Update
}

// [1300번대] 사장님 DB 처리 (Owner)
namespace CmdDBOwner {
    constexpr uint16_t REQ_DB_STORE_INFO    = 1300; // 매장 정보 조회 DB Select
    constexpr uint16_t REQ_DB_UPDATE_STORE  = 1301; // 매장 정보 수정 DB Update
    constexpr uint16_t REQ_DB_ADD_MENU      = 1302; // 메뉴 등록 DB Insert
    constexpr uint16_t REQ_DB_UPDATE_MENU   = 1303; // 메뉴 수정 DB Update
    constexpr uint16_t REQ_DB_ORDER_LIST    = 1304; // 접수 대기/진행 주문 목록 DB Select
    constexpr uint16_t REQ_DB_ACCEPT_ORDER  = 1305; // 주문 수락 상태 DB Update
    constexpr uint16_t REQ_DB_REJECT_ORDER  = 1306; // 주문 거절 상태 DB Update
    constexpr uint16_t REQ_DB_COOKING_DONE  = 1307; // 조리 완료 상태 DB Update
    constexpr uint16_t REQ_DB_SALES_STATS   = 1308; // 매출 통계 조회 DB Select (Group By 등)
    constexpr uint16_t REQ_DB_CHANGE_STATUS = 1309; // 영업 상태(오픈/마감) DB Update
}

// [1400번대] 라이더 DB 처리 (Rider)
namespace CmdDBRider {
    constexpr uint16_t REQ_DB_DISPATCH_LIST   = 1400; // 배차 리스트 조회 DB Select
    constexpr uint16_t REQ_DB_ACCEPT_DISPATCH = 1401; // 배차 수락 DB Update
    constexpr uint16_t REQ_DB_REJECT_DISPATCH = 1402; // 배차 거절 DB Update/Delete
    constexpr uint16_t REQ_DB_PICKUP_DONE     = 1403; // 픽업 완료 DB Update
    constexpr uint16_t REQ_DB_DELIVERY_DONE   = 1404; // 배달 완료 DB Update
    constexpr uint16_t REQ_DB_WORK_STATUS     = 1406; // 출퇴근 상태 DB Update
    constexpr uint16_t REQ_DB_SEND_GPS        = 1407; // 위치 정보 DB Update/Insert
    // ★ 인증/프로필 훅(Hook)용 프로토콜
    constexpr uint16_t REQ_DB_CREATE_PROFILE    = 1410;
    constexpr uint16_t REQ_DB_LOGIN_HOOK        = 1411;
    constexpr uint16_t REQ_DB_LOGOUT_HOOK       = 1412;
    constexpr uint16_t REQ_DB_GET_RIDER_PROFILE = 1413;
    constexpr uint16_t REQ_DB_MY_DISPATCHES     = 1414;
}

// [1500번대] 관리자 DB 처리 (Admin)
namespace CmdDBAdmin {
    constexpr uint16_t REQ_DB_SETTLEMENT_LIST = 1500; // 정산 목록 조회 DB Select
    constexpr uint16_t REQ_DB_SETTLEMENT_DET  = 1501; // 정산 상세 조회 DB Select
    constexpr uint16_t REQ_DB_SETTLEMENT_CONF = 1502; // 정산 확정 처리 DB Update
    constexpr uint16_t REQ_DB_MONITOR_ORDERS  = 1510; // 전체 대기/지연 주문 모니터링 DB Select
    constexpr uint16_t REQ_DB_RIDER_STATUS    = 1511; // 전체 라이더 위치/상태 DB Select
    constexpr uint16_t REQ_DB_FORCE_DISPATCH  = 1512; // 강제 배차 지정 DB Update
    constexpr uint16_t REQ_DB_FORCE_CANCEL    = 1513; // 배차 강제 취소 DB Update
    constexpr uint16_t REQ_DB_MANAGE_REVIEW   = 1520; // 악성 리뷰 블라인드/삭제 DB Update/Delete
}

// [1600번대] 채팅 DB 처리 (Chat)
namespace CmdDBChat {
    constexpr uint16_t REQ_DB_FIND_OR_CREATE_ROOM = 1600; // 채팅방 조회 또는 생성 DB Select/Insert
    constexpr uint16_t REQ_DB_QUERY_ROOM          = 1601; // 채팅방 유효성 확인 DB Select
    constexpr uint16_t REQ_DB_INSERT_MESSAGE      = 1602; // 메시지 저장 DB Insert
    constexpr uint16_t REQ_DB_QUERY_MESSAGES      = 1603; // 메시지 목록 조회 DB Select
}