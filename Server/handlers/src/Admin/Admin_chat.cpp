#include "AdminHandler.h"
#include "ChatDB.h"
#include "ChatRoomManager.h"
#include "MariaDBManager.h"
#include "MariaDB_AcceptManager.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// ★ AdminChat 생성자
AdminChat::AdminChat(BaseHandler& handler) : m_handler(handler) {}

// ============================================================
// 600: handleJoinRoom — 관리자가 기존 채팅방에 입장한다.
//
// 요청 JSON:
//   { "room_id": 7 }   ← 고객/라이더가 미리 만든 방의 ID
//
// 규칙:
//   - CUSTOMER_OWNER(사장님↔고객) 방은 FORBIDDEN — 절대 입장 불가
//   - CUSTOMER_ADMIN / RIDER_ADMIN 방만 허용
//   - 입장 성공 시 ChatRoomManager::joinRoom 으로 fd 등록 → 실시간 수신 가능
//   - 과거 메시지 100건 함께 응답
//
// 응답 JSON:
//   { "status":2000, "room_id":7, "messages":[...] }
// ============================================================
void AdminHandler::handleJoinRoom(Session* session,
                                   const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 채팅방 입장 (600)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        json req = jsonBody.empty() ? json::object() : json::parse(jsonBody);

        int adminId = getUserIdByFd(session->getFd());
        if (adminId <= 0) {
            sendError(session, CmdChat::REQ_CREATE_ROOM,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        int roomId = req.value("room_id", 0);
        if (roomId <= 0) {
            sendError(session, CmdChat::REQ_CREATE_ROOM,
                      Status::BAD_REQUEST, "room_id 가 필요합니다.");
            return;
        }

        // ── room_type 검증 ────────────────────────────────────
        auto& db   = MariaDBManager::getInstance();
        auto  rows = db.executeQuery(
            "SELECT room_type FROM chat_rooms "
            "WHERE room_id=" + std::to_string(roomId) +
            "  AND is_active=TRUE LIMIT 1");

        if (rows.empty()) {
            sendError(session, CmdChat::REQ_CREATE_ROOM,
                      Status::NOT_FOUND, "존재하지 않는 채팅방입니다.");
            return;
        }

        std::string roomType = rows[0].at("room_type");

        // 사장님↔고객 전용 방 — 관리자 진입 금지
        if (roomType == "CUSTOMER_OWNER") {
            sendError(session, CmdChat::REQ_CREATE_ROOM,
                      Status::FORBIDDEN,
                      "사장님↔고객 채팅방에는 관리자가 입장할 수 없습니다.");
            return;
        }

        if (roomType != "CUSTOMER_ADMIN" && roomType != "RIDER_ADMIN") {
            sendError(session, CmdChat::REQ_CREATE_ROOM,
                      Status::FORBIDDEN, "알 수 없는 채팅방 유형입니다.");
            return;
        }

        // ── ChatRoomManager 에 관리자 fd 등록 ────────────────
        ChatRoomManager::getInstance().joinRoom(
            roomId, session->getFd(), ClientType::ADMIN);

        std::cout << "[AdminChat] 관리자 입장 adminId=" << adminId
                  << " roomId=" << roomId
                  << " roomType=" << roomType << std::endl;

        // ── 과거 메시지 100건 함께 응답 ──────────────────────
        auto messages = ChatDB::getInstance().queryMessages(roomId, 100);

        json res;
        res["status"]   = Status::SUCCESS;
        res["room_id"]  = roomId;
        res["messages"] = json::array();
        for (const auto& m : messages) {
            json item;
            item["message_id"]  = m.messageId;
            item["sender_role"] = m.senderRole;
            item["content"]     = m.content;
            item["sent_at"]     = m.sentAt;
            res["messages"].push_back(item);
        }
        sendResponse(session, CmdChat::REQ_CREATE_ROOM, res);

    } catch (const std::exception& e) {
        std::cerr << "[AdminChat] handleJoinRoom 예외: " << e.what() << std::endl;
        sendError(session, CmdChat::REQ_CREATE_ROOM,
                  Status::SERVER_ERROR, "서버 오류");
    }
}// ============================================================
// 601: handleSendMsg — 관리자가 메시지를 전송한다.
// chat_messages 테이블에 INSERT하고 성공 응답을 보낸다.
// ============================================================
void AdminHandler::handleSendMsg(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 메시지 전송 (601)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        json req = json::parse(jsonBody);
        std::string roomId  = req.value("room_id", "");
        std::string message = req.value("message", "");

        // 파라미터 검증을 수행한다.
        if (roomId.empty() || message.empty()) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 메시지 전송 파라미터 부족" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdChat::REQ_SEND_MSG, Status::BAD_REQUEST, "room_id, message 필요");
            return;
        }

        // 현재 로그인된 관리자 userId를 가져온다.
        int adminUserId = getUserIdByFd(session->getFd());
        auto& db = MariaDBManager::getInstance();
        auto& dbEsc = MariaDB_AcceptManager::getInstance();

        // 메시지를 DB에 저장한다.
        std::string q =
            "INSERT INTO chat_messages (room_id, sender_id, message, is_admin, created_at) "
            "VALUES ('" + dbEsc.escapeStr(roomId) + "', " + std::to_string(adminUserId) +
            ", '" + dbEsc.escapeStr(message) + "', 1, NOW())";

        if (!db.executeUpdate(q)) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 메시지 저장 DB 실패" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "메시지 저장 실패");
            return;
        }

        // 성공 응답을 전송한다.
        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "전송 완료";
        sendResponse(session, CmdChat::REQ_SEND_MSG, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 메시지 전송 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 602: handleGetMsgs — 채팅 메시지를 조회한다.
// 지정된 room_id의 메시지를 시간순으로 가져온다.
// ============================================================
void AdminHandler::handleGetMsgs(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 메시지 조회 (602)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        json req = json::parse(jsonBody);
        std::string roomId = req.value("room_id", "");

        // 파라미터 검증을 수행한다.
        if (roomId.empty()) {
            sendError(session, CmdChat::REQ_GET_MSGS, Status::BAD_REQUEST, "room_id 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        auto& dbEsc = MariaDB_AcceptManager::getInstance();

        // 해당 채팅방의 메시지를 시간순으로 조회한다.
        std::string q =
            "SELECT message, is_admin, created_at "
            "FROM chat_messages "
            "WHERE room_id = '" + dbEsc.escapeStr(roomId) + "' "
            "ORDER BY created_at ASC "
            "LIMIT 500";

        DBResult rows = db.executeQuery(q);

        // 응답 JSON을 구성한다.
        json res;
        res["status"]   = Status::SUCCESS;
        res["messages"] = json::array();

        for (auto& row : rows)
        {
            json item;
            item["text"]     = row.count("message")    ? row.at("message")    : "";
            item["is_admin"] = row.count("is_admin")   ? (row.at("is_admin") == "1") : false;
            item["time"]     = row.count("created_at") ? row.at("created_at") : "";
            res["messages"].push_back(item);
        }

        sendResponse(session, CmdChat::REQ_GET_MSGS, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 메시지 조회 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 603: handleRoomList — 채팅방 목록을 조회한다.
// chat_rooms 테이블에서 목록을 가져오고 마지막 메시지를 포함한다.
// ============================================================
void AdminHandler::handleRoomList(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 채팅방 목록 조회 (603)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        auto& db = MariaDBManager::getInstance();

        // 채팅방 목록을 조회한다 (마지막 메시지 서브쿼리 포함).
        std::string q =
            "SELECT cr.room_id, "
            "       IFNULL(u.login_id, '') AS user_name, "
            "       IFNULL(u.role, '고객') AS role, "
            "       ( SELECT cm.message FROM chat_messages cm "
            "         WHERE cm.room_id = cr.room_id "
            "         ORDER BY cm.created_at DESC LIMIT 1 "
            "       ) AS last_message "
            "FROM chat_rooms cr "
            "LEFT JOIN users u ON cr.user_id = u.user_id "
            "ORDER BY cr.room_id DESC "
            "LIMIT 100";

        DBResult rows = db.executeQuery(q);

        // 응답 JSON을 구성한다.
        json res;
        res["status"] = Status::SUCCESS;
        res["rooms"]  = json::array();

        for (auto& row : rows)
        {
            json item;
            item["room_id"]      = row.count("room_id")      ? row.at("room_id")      : "";
            item["user_name"]    = row.count("user_name")     ? row.at("user_name")     : "";
            item["role"]         = row.count("role")          ? row.at("role")           : "고객";
            item["last_message"] = row.count("last_message")  ? row.at("last_message")  : "";
            res["rooms"].push_back(item);
        }

        sendResponse(session, CmdChat::REQ_ROOM_LIST, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 채팅방 목록 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdChat::REQ_ROOM_LIST, Status::SERVER_ERROR, "서버 오류");
    }
}