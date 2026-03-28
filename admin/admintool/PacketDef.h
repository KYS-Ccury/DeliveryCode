/**
 * PacketDef.h
 * ============================================================
 * 서버 Struct.h / Protocol.h 와 1:1 대응하는 클라이언트측 정의
 *
 * Header (7 bytes):
 *   [0]    clientType  : 1 byte
 *   [1~2]  protocol    : 2 bytes
 *   [3~6]  bodyLength  : 4 bytes
 *
 * ★ 수정사항:
 *   1) [200번대] 하트비트 프로토콜 추가
 *   2) WM_POLL_xxx 커스텀 메시지 정의 (폴링 → UI 전달용)
 * ============================================================
 */

#pragma once
#include <cstdint>

 // ============================================================
 // 헤더 크기 (서버 Struct.h 동일)
 // ============================================================
constexpr int HEADER_SIZE = 7;

// ============================================================
// Client Type (서버 Protocol.h → ClientType enum)
// ============================================================
constexpr uint8_t CLIENT_TYPE_CUSTOMER = 1;
constexpr uint8_t CLIENT_TYPE_OWNER = 2;
constexpr uint8_t CLIENT_TYPE_RIDER = 3;
constexpr uint8_t CLIENT_TYPE_ADMIN = 4;

// ============================================================
// Status 코드 (서버 Protocol.h → Status 네임스페이스)
// ============================================================
constexpr uint16_t STATUS_SUCCESS = 2000;
constexpr uint16_t STATUS_BAD_REQUEST = 4000;
constexpr uint16_t STATUS_UNAUTHORIZED = 4001;
constexpr uint16_t STATUS_FORBIDDEN = 4003;
constexpr uint16_t STATUS_NOT_FOUND = 4004;
constexpr uint16_t STATUS_SERVER_ERROR = 5000;

// ============================================================
// [100번대] 공통 (서버 CmdCommon)
// ============================================================
constexpr uint16_t CMD_SIGNUP = 100;
constexpr uint16_t CMD_LOGIN = 101;
constexpr uint16_t CMD_LOGOUT = 102;
constexpr uint16_t CMD_TOKEN_REFRESH = 103;
constexpr uint16_t CMD_GET_PROFILE = 104;
constexpr uint16_t CMD_WITHDRAW = 105;

// ============================================================
// [200번대] 하트비트 (서버 CmdHeartbeat)  ★ 추가
// ============================================================
constexpr uint16_t CMD_HEARTBEAT_REQ = 200;   // 클라 → 서버 (살아있어?)
constexpr uint16_t CMD_HEARTBEAT_RES = 201;   // 서버 → 클라 (살아있다)

// ============================================================
// [500번대] 관리자 (서버 CmdAdmin)
// ============================================================
constexpr uint16_t CMD_SETTLEMENT_LIST = 500;
constexpr uint16_t CMD_SETTLEMENT_DETAIL = 501;
constexpr uint16_t CMD_SETTLEMENT_CONFIRM = 502;
constexpr uint16_t CMD_ORDER_MONITOR = 510;
constexpr uint16_t CMD_RIDER_STATUS = 511;
constexpr uint16_t CMD_FORCE_DISPATCH = 512;
constexpr uint16_t CMD_FORCE_CANCEL = 513;
constexpr uint16_t CMD_MANAGE_REVIEW = 520;

// ============================================================
// [600번대] 채팅 (서버 CmdChat)
// ============================================================
constexpr uint16_t CMD_CREATE_ROOM = 600;
constexpr uint16_t CMD_SEND_MSG = 601;
constexpr uint16_t CMD_GET_MSGS = 602;
constexpr uint16_t CMD_ROOM_LIST = 603;
constexpr uint16_t CMD_NTF_RECV_MSG = 604;
constexpr uint16_t CMD_READ_RECEIPT = 605;

// ============================================================
// 패킷 헤더 구조체 (서버 Struct.h 동일)
// ============================================================
#pragma pack(push, 1)
struct PacketHeader
{
    uint8_t  clientType;
    uint16_t protocol;
    uint32_t bodyLength;
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == HEADER_SIZE,
    "PacketHeader must be 7 bytes (matching server Struct.h)");

// ============================================================
// ★ 폴링 → UI 전달용 커스텀 윈도우 메시지
//    WPARAM / LPARAM 사용법은 각 핸들러 주석 참조
// ============================================================
constexpr UINT WM_POLL_HEARTBEAT_OK = WM_APP + 100;  // 하트비트 응답 성공
constexpr UINT WM_POLL_HEARTBEAT_FAIL = WM_APP + 101;  // 하트비트 응답 실패 (연결 끊김)
constexpr UINT WM_POLL_NEW_MESSAGES = WM_APP + 102;  // 새 채팅 메시지 도착
constexpr UINT WM_POLL_ROOM_UPDATED = WM_APP + 103;  // 채팅방 목록 갱신됨