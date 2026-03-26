#include "ChatDB.h"
#include "Protocol.h"
#include <iostream>
#include <sstream>

// ── SQL 이스케이프 ─────────────────────────────────────────
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
//  주어진 (key, roomType) 쌍의 채팅방이 이미 존재하면 반환하고,
//  없으면 새로 생성 후 반환한다.
//  chat_rooms.order_id 컬럼을 범용 key 로 재활용한다.
// ============================================================
ChatDB::RoomResult ChatDB::findOrCreateRoom(int key, const std::string& roomType) {
    RoomResult result;
    auto& db = MariaDBManager::getInstance();

    // 기존 활성 방 조회
    auto rows = db.executeQuery(
        "SELECT room_id FROM chat_rooms "
        "WHERE order_id="  + std::to_string(key) +
        "  AND room_type='" + escape(roomType) + "'"
        "  AND is_active=TRUE LIMIT 1");

    if (!rows.empty()) {
        result.roomId = std::stoi(rows[0].at("room_id"));
        result.isNew  = false;
        result.ok     = true;
        return result;
    }

    // 없으면 새로 생성
    bool inserted = db.executeUpdate(
        "INSERT INTO chat_rooms (order_id, room_type, is_active) "
        "VALUES (" + std::to_string(key) + ",'" + escape(roomType) + "',TRUE)");

    if (!inserted) {
        std::cerr << "[ChatDB] findOrCreateRoom: INSERT 실패 (key="
                  << key << ", type=" << roomType << ")" << std::endl;
        return result; // result.ok == false
    }

    result.roomId = static_cast<int>(db.getLastInsertId());
    result.isNew  = true;
    result.ok     = true;
    return result;
}

// ============================================================
//  queryRoom
//  room_id 가 유효한(is_active=TRUE) 방인지 확인하고
//  기본 정보를 반환한다.
// ============================================================
ChatDB::RoomInfo ChatDB::queryRoom(int roomId) {
    RoomInfo info;
    auto& db = MariaDBManager::getInstance();

    auto rows = db.executeQuery(
        "SELECT room_id, order_id FROM chat_rooms "
        "WHERE room_id=" + std::to_string(roomId) +
        "  AND is_active=TRUE LIMIT 1");

    if (rows.empty()) return info; // found == false

    info.roomId  = roomId;
    info.orderId = rows[0].count("order_id") && !rows[0].at("order_id").empty()
                   ? std::stoi(rows[0].at("order_id")) : 0;
    info.found   = true;
    return info;
}

// ============================================================
//  insertMessage
//  chat_messages 에 메시지를 삽입하고
//  message_id 와 sent_at(HH:MM) 을 반환한다.
// ============================================================
ChatDB::SendMsgResult ChatDB::insertMessage(int roomId,
                                            int senderId,
                                            const std::string& content) {
    SendMsgResult result;
    auto& db = MariaDBManager::getInstance();

    bool ok = db.executeUpdate(
        "INSERT INTO chat_messages (room_id, sender_id, content) "
        "VALUES (" + std::to_string(roomId) +
        ","        + std::to_string(senderId) +
        ",'"       + escape(content) + "')");

    if (!ok) {
        std::cerr << "[ChatDB] insertMessage: INSERT 실패" << std::endl;
        return result; // ok == false
    }

    int msgId = static_cast<int>(db.getLastInsertId());

    // sent_at 조회 (HH:MM 포맷)
    auto ts = db.executeQuery(
        "SELECT DATE_FORMAT(sent_at,'%H:%i') AS t "
        "FROM chat_messages "
        "WHERE message_id=" + std::to_string(msgId) + " LIMIT 1");

    result.messageId = msgId;
    result.sentAt    = ts.empty() ? "" : ts[0].at("t");
    result.ok        = true;
    return result;
}

// ============================================================
//  queryMessages
//  room_id 에 속한 메시지를 오래된 순으로 최대 limit 건 반환한다.
// ============================================================
std::vector<ChatDB::ChatMessage> ChatDB::queryMessages(int roomId, int limit) {
    auto& db = MariaDBManager::getInstance();

    auto rows = db.executeQuery(
        "SELECT cm.message_id, u.role AS sender_role, "
        "       cm.content, DATE_FORMAT(cm.sent_at,'%H:%i') AS sent_at "
        "FROM chat_messages cm "
        "JOIN users u ON u.user_id = cm.sender_id "
        "WHERE cm.room_id=" + std::to_string(roomId) +
        " ORDER BY cm.sent_at ASC "
        "LIMIT " + std::to_string(limit));

    std::vector<ChatMessage> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        ChatMessage msg;
        msg.messageId  = row.count("message_id") && !row.at("message_id").empty()
                         ? std::stoi(row.at("message_id")) : 0;
        msg.senderRole = row.count("sender_role") ? row.at("sender_role") : "";
        msg.content    = row.count("content")     ? row.at("content")     : "";
        msg.sentAt     = row.count("sent_at")     ? row.at("sent_at")     : "";
        result.push_back(msg);
    }
    return result;
}

// ============================================================
//  process  —  MiddleHandler 라우팅 진입점
//  RiderDB::process / CommonDB::process 와 동일한 패턴
// ============================================================
nlohmann::json ChatDB::process(uint16_t dbProtocol, const nlohmann::json& reqJson) {
    nlohmann::json res;

    if (dbProtocol == CmdDBChat::REQ_DB_FIND_OR_CREATE_ROOM) {
        int key = reqJson.value("key", 0);
        std::string roomType = reqJson.value("room_type", "");
        auto result = getInstance().findOrCreateRoom(key, roomType);
        res["status"]  = result.ok ? Status::SUCCESS : Status::SERVER_ERROR;
        res["room_id"] = result.roomId;
        res["is_new"]  = result.isNew;
        return res;
    }

    if (dbProtocol == CmdDBChat::REQ_DB_QUERY_ROOM) {
        int roomId = reqJson.value("room_id", 0);
        auto info = getInstance().queryRoom(roomId);
        res["status"] = info.found ? Status::SUCCESS : Status::NOT_FOUND;
        res["found"]  = info.found;
        return res;
    }

    if (dbProtocol == CmdDBChat::REQ_DB_INSERT_MESSAGE) {
        int roomId   = reqJson.value("room_id",   0);
        int senderId = reqJson.value("sender_id", 0);
        std::string content = reqJson.value("content", "");
        auto result = getInstance().insertMessage(roomId, senderId, content);
        res["status"]     = result.ok ? Status::SUCCESS : Status::SERVER_ERROR;
        res["message_id"] = result.messageId;
        res["sent_at"]    = result.sentAt;
        return res;
    }

    if (dbProtocol == CmdDBChat::REQ_DB_QUERY_MESSAGES) {
        int roomId = reqJson.value("room_id", 0);
        int limit  = reqJson.value("limit", 100);
        auto msgs = getInstance().queryMessages(roomId, limit);
        res["status"]   = Status::SUCCESS;
        res["messages"] = nlohmann::json::array();
        for (const auto& m : msgs) {
            nlohmann::json item;
            item["message_id"] = m.messageId;
            item["sender"]     = m.senderRole;
            item["content"]    = m.content;
            item["sent_at"]    = m.sentAt;
            res["messages"].push_back(item);
        }
        return res;
    }

    res["status"]  = Status::SERVER_ERROR;
    res["message"] = "Unknown ChatDB Protocol";
    return res;
}
