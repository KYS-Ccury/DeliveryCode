// ============================================================
//  ChatHandler.cpp [최종 수정본]
// ============================================================
#include "ChatHandler.h"
#include "EpollServer.h"
#include "RiderHandler.h"
#include "AdminHandler.h" // AdminHandler 싱글톤 접근을 위해 추가
#include "Session.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ─── 헬퍼 함수 ─────────────────────────────────────────────
// BaseHandler의 escapeStr을 사용하거나, 없으면 아래 static 함수 유지
static std::string escStr(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// 에러 전송 헬퍼 (ClientType을 인자로 받아 유연하게 대응)
static void sendErr(Session* s, uint16_t proto, uint16_t code, const std::string& msg, ClientType ct) {
    json r; r["status"] = code; r["message"] = msg;
    s->sendPacket(static_cast<uint8_t>(ct), proto, r.dump());
}

// ─── 메인 디스패치 ────────────────────────────────────────
void ChatHandler::process(Session* session, uint16_t protocol,
                          const std::string& jsonBody,
                          ClientType clientType) {
    switch (protocol) {
        case CmdChat::REQ_CREATE_ROOM: handleCreateRoom(session, jsonBody, clientType); break;
        case CmdChat::REQ_SEND_MSG:    handleSendMsg   (session, jsonBody, clientType); break;
        case CmdChat::REQ_GET_MSGS:    handleGetMsgs   (session, jsonBody, clientType); break;
        default: break;
    }
}

// ─────────────────────────────────────────────────────────
//  600: 채팅방 생성 또는 기존 방 조회
// ─────────────────────────────────────────────────────────
void ChatHandler::handleCreateRoom(Session* session, const std::string& jsonBody,
                                   ClientType clientType) {
    try {
        json req = jsonBody.empty() ? json::object() : json::parse(jsonBody);
        auto& db = MariaDBManager::getInstance();

        int riderId = 0;
        if (clientType == ClientType::RIDER) {
            // ★ 수정: getInstance() 사용 및 session->getFd() 인자 전달
            riderId = RiderHandler::getInstance().getUserIdByFd(session->getFd());
        } else {
            riderId = req.value("rider_id", 0);
        }

        if (riderId <= 0) {
            sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::BAD_REQUEST, "사용자 식별 실패", clientType);
            return;
        }

        std::string checkQ =
            "SELECT room_id FROM chat_rooms "
            "WHERE order_id = " + std::to_string(riderId) +
            "  AND room_type = 'RIDER_ADMIN' "
            "  AND is_active = TRUE LIMIT 1";
        DBResult rows = db.executeQuery(checkQ);

        int roomId = 0;
        bool isNew = false;
        if (!rows.empty()) {
            roomId = std::stoi(rows[0].at("room_id"));
        } else {
            bool ok = db.executeUpdate(
                "INSERT INTO chat_rooms (order_id, room_type, is_active) "
                "VALUES (" + std::to_string(riderId) + ", 'RIDER_ADMIN', TRUE)");
            if (!ok) {
                sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::SERVER_ERROR, "방 생성 실패", clientType);
                return;
            }
            roomId = static_cast<int>(db.getLastInsertId());
            isNew  = true;
        }

        json res;
        res["status"]  = Status::SUCCESS;
        res["room_id"] = roomId;
        res["is_new"]  = isNew;

        session->sendPacket(static_cast<uint8_t>(clientType), CmdChat::REQ_CREATE_ROOM, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleCreateRoom] 예외: " << e.what() << std::endl;
        sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::SERVER_ERROR, "서버 오류", clientType);
    }
}

// ─────────────────────────────────────────────────────────
//  601: 메시지 전송 + 상대방에게 실시간 Push (604)
// ─────────────────────────────────────────────────────────
void ChatHandler::handleSendMsg(Session* session, const std::string& jsonBody,
                                ClientType clientType) {
    try {
        json req = json::parse(jsonBody);
        int    roomId  = req.value("room_id", 0);
        std::string msg = req.value("message", "");

        if (roomId <= 0 || msg.empty()) {
            sendErr(session, CmdChat::REQ_SEND_MSG, Status::BAD_REQUEST, "파라미터 누락", clientType);
            return;
        }

        int senderId = 0;
        std::string senderRole;

        // ★ 수정: getInstance() 및 getUserIdByFd(fd)로 통일
        if (clientType == ClientType::RIDER) {
            senderId   = RiderHandler::getInstance().getUserIdByFd(session->getFd());
            senderRole = "RIDER";
        } else if (clientType == ClientType::ADMIN) {
            // AdminHandler도 싱글톤이라고 가정
            senderId   = AdminHandler::getInstance().getUserIdByFd(session->getFd());
            senderRole = "ADMIN";
        }

        if (senderId <= 0) {
            sendErr(session, CmdChat::REQ_SEND_MSG, Status::UNAUTHORIZED, "로그인 필요", clientType);
            return;
        }

        auto& db = MariaDBManager::getInstance();

        DBResult room = db.executeQuery(
            "SELECT room_id, order_id FROM chat_rooms "
            "WHERE room_id = " + std::to_string(roomId) +
            "  AND is_active = TRUE LIMIT 1");
        
        if (room.empty()) {
            sendErr(session, CmdChat::REQ_SEND_MSG, Status::NOT_FOUND, "채팅방 없음", clientType);
            return;
        }

        db.executeUpdate(
            "INSERT INTO chat_messages (room_id, sender_id, content) "
            "VALUES (" + std::to_string(roomId) + ", "
                      + std::to_string(senderId) + ", '"
                      + escStr(msg) + "')");
        
        int msgId = static_cast<int>(db.getLastInsertId());
        DBResult ts = db.executeQuery("SELECT DATE_FORMAT(sent_at, '%H:%i') AS t FROM chat_messages WHERE message_id = " + std::to_string(msgId));
        std::string sentAt = ts.empty() ? "" : ts[0].at("t");

        // 발신자에게 응답
        json res;
        res["status"]     = Status::SUCCESS;
        res["message_id"] = msgId;
        res["sent_at"]    = sentAt;
        session->sendPacket(static_cast<uint8_t>(clientType), CmdChat::REQ_SEND_MSG, res.dump());

        // ── 상대방에게 Push (604) ─────────────────────────
        json push;
        push["room_id"]  = roomId;
        push["sender"]   = senderRole;
        push["message"]  = msg;
        push["sent_at"]  = sentAt;
        std::string pushBody = push.dump();

        int riderIdInRoom = std::stoi(room[0].at("order_id"));

        if (clientType == ClientType::RIDER) {
            // 라이더가 보냄 → 관리자에게 Push (관리자 ID가 1번이라고 가정하거나 세션 검색)
            int adminFd = AdminHandler::getInstance().getFdByUserId(1); 
            if (adminFd > 0 && EpollServer::s_instance) {
                auto adminSess = EpollServer::s_instance->getSession(adminFd);
                if (adminSess)
                    adminSess->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdChat::NTF_RECV_MSG, pushBody);
            }
        } else {
            // 관리자가 보냄 → 라이더에게 Push
            // ★ 수정: RiderHandler::getInstance().getFdByUserId() 사용
            int riderToFd = RiderHandler::getInstance().getFdByUserId(riderIdInRoom);
            if (riderToFd > 0 && EpollServer::s_instance) {
                auto riderSess = EpollServer::s_instance->getSession(riderToFd);
                if (riderSess)
                    riderSess->sendPacket(static_cast<uint8_t>(ClientType::RIDER), CmdChat::NTF_RECV_MSG, pushBody);
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[handleSendMsg] 예외: " << e.what() << std::endl;
        sendErr(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "서버 오류", clientType);
    }
}

// ─────────────────────────────────────────────────────────
//  602: 채팅 히스토리 조회
// ─────────────────────────────────────────────────────────
void ChatHandler::handleGetMsgs(Session* session, const std::string& jsonBody,
                                ClientType clientType) {
    try {
        json req    = json::parse(jsonBody);
        int  roomId = req.value("room_id", 0);
        if (roomId <= 0) {
            sendErr(session, CmdChat::REQ_GET_MSGS, Status::BAD_REQUEST, "room_id 누락", clientType);
            return;
        }

        auto& db = MariaDBManager::getInstance();

        std::string q =
            "SELECT cm.message_id, u.role AS sender_role, "
            "       cm.content, "
            "       DATE_FORMAT(cm.sent_at, '%H:%i') AS sent_at "
            "FROM chat_messages cm "
            "JOIN users u ON u.user_id = cm.sender_id "
            "WHERE cm.room_id = " + std::to_string(roomId) +
            " ORDER BY cm.sent_at ASC LIMIT 100";

        DBResult rows = db.executeQuery(q);

        json res;
        res["status"]   = Status::SUCCESS;
        res["messages"] = json::array();

        for (const auto& row : rows) {
            json item;
            item["sender"]   = row.at("sender_role");
            item["message"]  = row.at("content");
            item["sent_at"]  = row.at("sent_at");
            res["messages"].push_back(item);
        }

        session->sendPacket(static_cast<uint8_t>(clientType), CmdChat::REQ_GET_MSGS, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleGetMsgs] 예외: " << e.what() << std::endl;
        sendErr(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "서버 오류", clientType);
    }
}