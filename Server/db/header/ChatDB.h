#pragma once
// ============================================================
//  ChatDB.h  (CUSTOMER_ADMIN 지원 버전)
//
//  변경 사항:
//    findOrCreateRoom 이 roomType 에 따라
//    RIDER_ADMIN    → rider_id    컬럼 사용
//    CUSTOMER_ADMIN → customer_id 컬럼 사용
//    CUSTOMER_OWNER → order_id    컬럼 사용
//    그 외           → order_id   컬럼 사용 (기존 동작 유지)
// ============================================================
#include "MariaDBManager.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ChatDB {
public:
    static ChatDB& getInstance() {
        static ChatDB inst;
        return inst;
    }

    // MiddleHandler 라우팅 진입점
    static nlohmann::json process(uint16_t dbProtocol,
                                  const nlohmann::json& reqJson);

    // ── 채팅방 ────────────────────────────────────────────

    struct RoomResult {
        int  roomId = 0;
        bool isNew  = false;
        bool ok     = false;
    };

    // roomType 에 따라 key 가 저장될 컬럼이 달라진다.
    //   RIDER_ADMIN    → rider_id    (기존)
    //   CUSTOMER_ADMIN → customer_id (신규)
    //   CUSTOMER_OWNER → order_id    (기존)
    //   그 외           → order_id    (기존 default)
    RoomResult findOrCreateRoom(int key, const std::string& roomType);

    struct RoomInfo {
        int  roomId  = 0;
        int  orderId = 0;
        bool found   = false;
    };
    RoomInfo queryRoom(int roomId);

    // ── 메시지 ────────────────────────────────────────────

    struct SendMsgResult {
        int         messageId = 0;
        std::string sentAt;
        bool        ok        = false;
    };
    SendMsgResult insertMessage(int roomId, int senderId,
                                const std::string& content);

    struct ChatMessage {
        int         messageId  = 0;
        std::string senderRole;
        std::string content;
        std::string sentAt;
    };
    std::vector<ChatMessage> queryMessages(int roomId, int limit = 100);

private:
    ChatDB() = default;
    static std::string escape(const std::string& s);
};
