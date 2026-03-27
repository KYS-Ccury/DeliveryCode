// ============================================================
//  ChatDB.cpp  (CUSTOMER_ADMIN 지원 버전)
//
//  findOrCreateRoom 변경 내용:
//    roomType == "RIDER_ADMIN"    → rider_id    컬럼
//    roomType == "CUSTOMER_ADMIN" → customer_id 컬럼  ★ 신규
//    그 외 (CUSTOMER_OWNER 등)    → order_id    컬럼
// ============================================================
#include "ChatDB.h"
#include "Protocol.h"
#include <iostream>
#include <sstream>

std::string ChatDB::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// ============================================================
//  findOrCreateRoom
//
//  roomType 별 사용 컬럼:
//    "RIDER_ADMIN"    → rider_id
//    "CUSTOMER_ADMIN" → customer_id
//    그 외             → order_id
// ============================================================
ChatDB::RoomResult ChatDB::findOrCreateRoom(int key,
                                             const std::string& roomType) {
    RoomResult result;
    auto& db = MariaDBManager::getInstance();

    // ── 컬럼명 결정 ──────────────────────────────────────────
    std::string colName;
    if (roomType == "RIDER_ADMIN")
        colName = "rider_id";
    else if (roomType == "CUSTOMER_ADMIN")
        colName = "customer_id";
    else
        colName = "order_id";   // CUSTOMER_OWNER, 기타

    // ── 기존 활성 방 조회 ────────────────────────────────────
    auto rows = db.executeQuery(
        "SELECT room_id FROM chat_rooms "
        "WHERE " + colName + "="     + std::to_string(key) +
        "  AND room_type='"          + escape(roomType) + "'"
        "  AND is_active=TRUE LIMIT 1");

    if (!rows.empty()) {
        result.roomId = std::stoi(rows[0].at("room_id"));
        result.isNew  = false;
        result.ok     = true;
        return result;
    }

    // ── 새 방 생성 ───────────────────────────────────────────
    bool inserted = db.executeUpdate(
        "INSERT INTO chat_rooms (" + colName + ", room_type, is_active) "
        "VALUES (" + std::to_string(key) +
        ",'" + escape(roomType) + "',TRUE)");

    if (!inserted) {
        std::cerr << "[ChatDB] findOrCreateRoom: INSERT 실패 (key="
                  << key << ", type=" << roomType << ")" << std::endl;
        return result;
    }

    result.roomId = static_cast<int>(db.getLastInsertId());
    result.isNew  = true;
    result.ok     = true;
    return result;
}

// ============================================================
//  queryRoom
// ============================================================
ChatDB::RoomInfo ChatDB::queryRoom(int roomId) {
    RoomInfo info;
    auto& db = MariaDBManager::getInstance();

    auto rows = db.executeQuery(
        "SELECT room_id, order_id FROM chat_rooms "
        "WHERE room_id=" + std::to_string(roomId) +
        "  AND is_active=TRUE LIMIT 1");

    if (rows.empty()) return info;

    info.roomId  = roomId;
    info.orderId = rows[0].count("order_id") && !rows[0].at("order_id").empty()
                   ? std::stoi(rows[0].at("order_id")) : 0;
    info.found   = true;
    return info;
}

// ============================================================
//  insertMessage
// ============================================================
ChatDB::SendMsgResult ChatDB::insertMessage(int roomId,
                                             int senderId,
                                             const std::string& content) {
    SendMsgResult result;
    auto& db = MariaDBManager::getInstance();

    bool ok = db.executeUpdate(
        "INSERT INTO chat_messages (room_id, sender_id, content) "
        "VALUES (" + std::to_string(roomId)   +
        ","        + std::to_string(senderId) +
        ",'"       + escape(content) + "')");

    if (!ok) {
        std::cerr << "[ChatDB] insertMessage: INSERT 실패" << std::endl;
        return result;
    }

    result.messageId = static_cast<int>(db.getLastInsertId());

    auto ts = db.executeQuery(
        "SELECT DATE_FORMAT(sent_at, '%H:%i') AS t "
        "FROM chat_messages WHERE message_id=" +
        std::to_string(result.messageId));

    result.sentAt = ts.empty() ? "" : ts[0].at("t");
    result.ok     = true;
    return result;
}

// ============================================================
//  queryMessages
// ============================================================
std::vector<ChatDB::ChatMessage> ChatDB::queryMessages(int roomId,
                                                        int limit) {
    std::vector<ChatMessage> list;
    auto& db = MariaDBManager::getInstance();

    auto rows = db.executeQuery(
        "SELECT cm.message_id, "
        "       UPPER(u.role) AS sender_role, "   // CUSTOMER / ADMIN / OWNER / RIDER
        "       cm.content, "
        "       DATE_FORMAT(cm.sent_at, '%H:%i') AS sent_at "
        "FROM chat_messages cm "
        "JOIN users u ON u.user_id = cm.sender_id "
        "WHERE cm.room_id=" + std::to_string(roomId) +
        " ORDER BY cm.sent_at ASC "
        " LIMIT " + std::to_string(limit));

    for (const auto& row : rows) {
        ChatMessage m;
        m.messageId  = row.count("message_id") ? std::stoi(row.at("message_id")) : 0;
        m.senderRole = row.count("sender_role") ? row.at("sender_role") : "";
        m.content    = row.count("content")     ? row.at("content")     : "";
        m.sentAt     = row.count("sent_at")     ? row.at("sent_at")     : "";
        list.push_back(m);
    }
    return list;
}
