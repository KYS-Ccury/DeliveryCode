// ============================================================
//  ChatHandler.cpp  [기능 구현부]
//  라이더 ↔ 관리자 실시간 채팅
//
//  프로토콜:
//    600 : REQ_CREATE_ROOM  - 채팅방 생성/조회
//    601 : REQ_SEND_MSG     - 메시지 전송
//    602 : REQ_GET_MSGS     - 메시지 히스토리 조회
//    604 : NTF_RECV_MSG     - 실시간 Push (서버→클라이언트)
// ============================================================
#include "ChatHandler.h"
#include "EpollServer.h"
#include "RiderHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mutex>

using json = nlohmann::json;

// ─── 관리자 세션 관리 ─────────────────────────────────────
std::unordered_map<int, int> ChatHandler::s_fdToAdmin;
std::unordered_map<int, int> ChatHandler::s_adminToFd;
std::mutex                   ChatHandler::s_adminMtx;

void ChatHandler::registerAdmin(int fd, int adminId) {
    std::lock_guard<std::mutex> lk(s_adminMtx);
    s_fdToAdmin[fd]      = adminId;
    s_adminToFd[adminId] = fd;
}

void ChatHandler::unregisterAdmin(int fd) {
    std::lock_guard<std::mutex> lk(s_adminMtx);
    auto it = s_fdToAdmin.find(fd);
    if (it != s_fdToAdmin.end()) {
        s_adminToFd.erase(it->second);
        s_fdToAdmin.erase(it);
    }
}

int ChatHandler::getAdminIdByFd(int fd) {
    std::lock_guard<std::mutex> lk(s_adminMtx);
    auto it = s_fdToAdmin.find(fd);
    return (it != s_fdToAdmin.end()) ? it->second : 0;
}

int ChatHandler::getAdminFd() {
    // 현재 접속된 관리자 중 첫 번째 반환
    std::lock_guard<std::mutex> lk(s_adminMtx);
    if (!s_adminToFd.empty())
        return s_adminToFd.begin()->second;
    return -1;
}

// ─── 헬퍼 ─────────────────────────────────────────────────
static std::string escStr(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

static void sendErr(Session* s, uint16_t proto,
                    uint16_t code, const std::string& msg) {
    json r; r["status"] = code; r["message"] = msg;
    s->sendPacket(static_cast<uint8_t>(ClientType::RIDER), proto, r.dump());
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
//  요청: { "rider_id": N }  (관리자가 특정 라이더와 채팅 시작할 때도 동일)
//  응답: { "status":2000, "room_id": N, "is_new": true/false }
// ─────────────────────────────────────────────────────────
void ChatHandler::handleCreateRoom(Session* session, const std::string& jsonBody,
                                   ClientType clientType) {
    try {
        json req = jsonBody.empty() ? json::object() : json::parse(jsonBody);
        auto& db = MariaDBManager::getInstance();

        // 라이더 ID 결정
        int riderId = 0;
        if (clientType == ClientType::RIDER) {
            riderId = RiderHandler::getRiderIdByFd(session->getFd());
        } else {
            riderId = req.value("rider_id", 0);
        }
        if (riderId <= 0) {
            sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::BAD_REQUEST, "rider_id 누락");
            return;
        }

        // 이미 활성화된 방이 있는지 확인
        // chat_rooms 테이블: order_id 대신 rider_id 기반으로 관리자 채팅방 운용
        // room_type = 'RIDER_ADMIN' 으로 구분
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
            // 새 방 생성 (order_id 컬럼을 rider_id로 활용)
            bool ok = db.executeUpdate(
                "INSERT INTO chat_rooms (order_id, room_type, is_active) "
                "VALUES (" + std::to_string(riderId) + ", 'RIDER_ADMIN', TRUE)");
            if (!ok) {
                sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::SERVER_ERROR, "방 생성 실패");
                return;
            }
            roomId = static_cast<int>(db.getLastInsertId());
            isNew  = true;
        }

        json res;
        res["status"]  = Status::SUCCESS;
        res["room_id"] = roomId;
        res["is_new"]  = isNew;

        uint8_t ct = static_cast<uint8_t>(clientType);
        session->sendPacket(ct, CmdChat::REQ_CREATE_ROOM, res.dump());

        std::cout << "[Chat] 채팅방 " << (isNew ? "생성" : "재사용")
                  << ": room_id=" << roomId
                  << " rider_id=" << riderId << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleCreateRoom] 예외: " << e.what() << std::endl;
        sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  601: 메시지 전송 + 상대방에게 실시간 Push (604)
//  요청: { "room_id": N, "message": "..." }
//  응답: { "status":2000, "message_id": N, "sent_at": "..." }
//  Push(604): { "room_id":N, "sender":"RIDER"/"ADMIN",
//               "message":"...", "sent_at":"..." }
// ─────────────────────────────────────────────────────────
void ChatHandler::handleSendMsg(Session* session, const std::string& jsonBody,
                                ClientType clientType) {
    try {
        json req = json::parse(jsonBody);
        int    roomId  = req.value("room_id", 0);
        std::string msg = req.value("message", "");

        if (roomId <= 0 || msg.empty()) {
            sendErr(session, CmdChat::REQ_SEND_MSG, Status::BAD_REQUEST, "파라미터 누락");
            return;
        }

        // 발신자 ID 결정
        int senderId = 0;
        std::string senderRole;
        if (clientType == ClientType::RIDER) {
            senderId   = RiderHandler::getRiderIdByFd(session->getFd());
            senderRole = "RIDER";
        } else if (clientType == ClientType::ADMIN) {
            senderId   = getAdminIdByFd(session->getFd());
            senderRole = "ADMIN";
        }
        if (senderId <= 0) {
            sendErr(session, CmdChat::REQ_SEND_MSG, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 채팅방 유효성 확인
        DBResult room = db.executeQuery(
            "SELECT room_id, order_id FROM chat_rooms "
            "WHERE room_id = " + std::to_string(roomId) +
            "  AND is_active = TRUE LIMIT 1");
        if (room.empty()) {
            sendErr(session, CmdChat::REQ_SEND_MSG, Status::NOT_FOUND, "채팅방 없음");
            return;
        }

        // 메시지 저장
        db.executeUpdate(
            "INSERT INTO chat_messages (room_id, sender_id, content) "
            "VALUES (" + std::to_string(roomId) + ", "
                      + std::to_string(senderId) + ", '"
                      + escStr(msg) + "')");
        int msgId = static_cast<int>(db.getLastInsertId());

        // 전송 시각 조회
        DBResult ts = db.executeQuery(
            "SELECT DATE_FORMAT(sent_at, '%H:%i') AS t "
            "FROM chat_messages WHERE message_id = " + std::to_string(msgId));
        std::string sentAt = ts.empty() ? "" : ts[0].at("t");

        // 발신자에게 응답
        json res;
        res["status"]     = Status::SUCCESS;
        res["message_id"] = msgId;
        res["sent_at"]    = sentAt;
        session->sendPacket(static_cast<uint8_t>(clientType),
                            CmdChat::REQ_SEND_MSG, res.dump());

        // ── 상대방에게 Push (604) ─────────────────────────
        json push;
        push["room_id"]  = roomId;
        push["sender"]   = senderRole;
        push["message"]  = msg;
        push["sent_at"]  = sentAt;
        std::string pushBody = push.dump();

        int riderId = std::stoi(room[0].at("order_id")); // order_id = rider_id

        if (clientType == ClientType::RIDER) {
            // 라이더가 보냄 → 관리자에게 Push
            int adminFd = getAdminFd();
            if (adminFd > 0 && EpollServer::s_instance) {
                auto adminSess = EpollServer::s_instance->getSession(adminFd);
                if (adminSess)
                    adminSess->sendPacket(static_cast<uint8_t>(ClientType::ADMIN),
                                          CmdChat::NTF_RECV_MSG, pushBody);
            }
        } else {
            // 관리자가 보냄 → 라이더에게 Push
            int riderToFd = -1;
            {
                // RiderHandler의 s_riderToFd는 private이므로 getRiderIdByFd 역방향 조회
                // 대신 DB에서 rider_id로 세션 접근
                // 간단히: 모든 라이더 세션을 순회하지 않고 room의 rider_id로 직접 조회
                // RiderHandler에 getRiderFdById 추가 필요 → 여기서는 직접 구현
            }
            // RiderHandler에 공개 메서드로 추가
            riderToFd = RiderHandler::getRiderFdById(riderId);
            if (riderToFd > 0 && EpollServer::s_instance) {
                auto riderSess = EpollServer::s_instance->getSession(riderToFd);
                if (riderSess)
                    riderSess->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                          CmdChat::NTF_RECV_MSG, pushBody);
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[handleSendMsg] 예외: " << e.what() << std::endl;
        sendErr(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  602: 채팅 히스토리 조회
//  요청: { "room_id": N }
//  응답: { "status":2000, "messages": [...] }
// ─────────────────────────────────────────────────────────
void ChatHandler::handleGetMsgs(Session* session, const std::string& jsonBody,
                                ClientType clientType) {
    try {
        json req    = json::parse(jsonBody);
        int  roomId = req.value("room_id", 0);
        if (roomId <= 0) {
            sendErr(session, CmdChat::REQ_GET_MSGS, Status::BAD_REQUEST, "room_id 누락");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 발신자 role 조회를 위해 users.role도 JOIN
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

        // 현재 사용자 ID
        int myId = (clientType == ClientType::RIDER)
                   ? RiderHandler::getRiderIdByFd(session->getFd())
                   : getAdminIdByFd(session->getFd());

        for (const auto& row : rows) {
            json item;
            std::string role = row.at("sender_role");
            // RIDER → "RIDER", ADMIN → "ADMIN"
            item["sender"]   = role;
            item["message"]  = row.at("content");
            item["sent_at"]  = row.at("sent_at");
            res["messages"].push_back(item);
        }

        session->sendPacket(static_cast<uint8_t>(clientType),
                            CmdChat::REQ_GET_MSGS, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleGetMsgs] 예외: " << e.what() << std::endl;
        sendErr(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "서버 오류");
    }
}
