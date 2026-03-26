#pragma once
#include "MariaDBManager.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// ============================================================
//  ChatDB  —  채팅 관련 DB 쿼리 전담 클래스
//  핸들러(Handler) 계층에서는 이 클래스만 호출하고,
//  SQL 문자열은 이 클래스 내부에서만 관리한다.
// ============================================================
class ChatDB {
public:
    static ChatDB& getInstance() {
        static ChatDB inst;
        return inst;
    }

    // MiddleHandler 라우팅 진입점 (RiderDB::process 와 동일한 패턴)
    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);

    // ── 채팅방 ────────────────────────────────────────────

    struct RoomResult {
        int  roomId = 0;
        bool isNew  = false;
        bool ok     = false;
    };

    // room_type : 'RIDER_ADMIN' | 'CUSTOMER_OWNER' | 'CUSTOMER_ADMIN' 등
    // key       : room_type 안에서 유일한 식별자 (riderId, customerId 등)
    RoomResult findOrCreateRoom(int key, const std::string& roomType);

    // room_id 유효성 확인 (is_active=TRUE)
    struct RoomInfo {
        int  roomId  = 0;
        int  orderId = 0;
        bool found   = false;
    };
    RoomInfo queryRoom(int roomId);

    // ── 메시지 ────────────────────────────────────────────

    struct SendMsgResult {
        int         messageId = 0;
        std::string sentAt;   // 'HH:MM' 포맷
        bool        ok        = false;
    };

    // chat_messages INSERT → 삽입된 message_id 와 sent_at 반환
    SendMsgResult insertMessage(int roomId, int senderId, const std::string& content);

    struct ChatMessage {
        int         messageId  = 0;
        std::string senderRole; // users.role
        std::string content;
        std::string sentAt;     // 'HH:MM' 포맷
    };

    // 해당 room_id 의 최근 메시지 목록 (오래된 순, 최대 100건)
    std::vector<ChatMessage> queryMessages(int roomId, int limit = 100);

private:
    ChatDB() = default;

    // SQL 이스케이프 헬퍼
    static std::string escape(const std::string& s);
};
