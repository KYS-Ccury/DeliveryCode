// ============================================================
//  Customer_Chat.cpp  [고객 채팅 핸들러]
//
//  ▶ 역할 분리 원칙 (RiderChatHandler 와 동일 패턴)
//    Handler  계층 : 파라미터 검증, 흐름 제어
//    ChatDB   계층 : 모든 SQL 쿼리  (영속 저장)
//    ChatRoomManager : 실시간 브로드캐스트 (서브 스레드 1개)
//
//  ▶ 지원하는 채팅방 유형
//    CUSTOMER_ADMIN  : 고객 ↔ 관리자  (마이페이지 > 관리자 채팅)
//    CUSTOMER_OWNER  : 고객 ↔ 사장님  (배달현황 > 가게에 문의)
//
//  ▶ 처리 프로토콜
//    600  REQ_CREATE_ROOM  — 채팅방 생성 또는 기존 방 재입장
//    601  REQ_SEND_MSG     — 메시지 전송 + 브로드캐스트
//    602  REQ_GET_MSGS     — 과거 메시지 100건 조회
//
//  ▶ 채팅방 생성(600) 흐름
//    1. target_type 파라미터로 방 유형 결정
//       "admin"  → CUSTOMER_ADMIN, key = customerId
//       "owner"  → CUSTOMER_OWNER, key = orderId
//    2. ChatDB::findOrCreateRoom → DB 방 조회/생성
//    3. ChatRoomManager::joinRoom → 서브 스레드 감시 맵 등록
//
//  ▶ 메시지 전송(601) 흐름
//    1. ChatDB::insertMessage  → DB 영속 저장
//    2. ChatRoomManager::enqueueMessage → 서브 스레드 큐 삽입
//       서브 스레드가 같은 방 참가자(고객 + 상대방) 전원에게 Push
//
//  ▶ DB 스키마 전제조건
//    chat_rooms.order_id    NULL 허용  (ALTER 필요 → chat_schema_alter.sql)
//    chat_rooms.customer_id BIGINT NULL (CUSTOMER_ADMIN 방 키)
//    chat_rooms.owner_id    BIGINT NULL (미래 직접 채팅 대비)
// ============================================================
#include "CustomerHandler.h"
#include "ChatDB.h"
#include "ChatRoomManager.h"
#include "ChatUtil.h"
#include "AdminHandler.h"     // 관리자 fd 조회
#include "OwnerHandler.h"     // 사장님 fd 조회
#include "Protocol.h"
#include "Session.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ============================================================
//  handleChatCreateRoom  (REQ_CREATE_ROOM = 600)
//
//  요청 JSON:
//    { "target_type": "admin" }                     ← 관리자 채팅
//    { "target_type": "owner", "order_id": 42 }     ← 사장님 채팅
//
//  응답 JSON:
//    { "status": 2000, "room_id": 7, "is_new": true }
// ============================================================
void CustomerHandler::handleChatCreateRoom(Session* session,
                                            const std::string& jsonBody) {
    try {
        json req = jsonBody.empty() ? json::object() : json::parse(jsonBody);

        // ── 로그인 확인 ───────────────────────────────────────
        int customerId = getUserIdByFd(session->getFd());
        if (customerId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                              Status::UNAUTHORIZED, "로그인이 필요합니다.",
                              ClientType::CUSTOMER);
            return;
        }

        std::string targetType = req.value("target_type", "");

        // ── 1. 채팅 방 유형 결정 및 DB 조회/생성 ─────────────
        ChatDB::RoomResult roomResult;

        if (targetType == "admin") {
            // 고객-관리자: customer_id 컬럼 기준
            // findOrCreateRoom 의 roomType = "CUSTOMER_ADMIN", key = customerId
            roomResult = ChatDB::getInstance()
                             .findOrCreateRoom(customerId, "CUSTOMER_ADMIN");

        } else if (targetType == "owner") {
            // 고객-사장님: order_id 기준으로 채팅방을 구분한다.
            // order_id 가 다르면 다른 채팅방 → 가게별 내역 분리 보장.
            // order_id 없이 방을 만들면 어느 가게 채팅인지 알 수 없으므로 거부.
            int orderId = req.value("order_id", 0);
            if (orderId <= 0) {
                ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                                  Status::BAD_REQUEST,
                                  "owner 채팅에는 order_id 가 필요합니다.",
                                  ClientType::CUSTOMER);
                return;
            }
            roomResult = ChatDB::getInstance()
                             .findOrCreateRoom(orderId, "CUSTOMER_OWNER");

        } else {
            ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                              Status::BAD_REQUEST,
                              "target_type 은 'admin' 또는 'owner' 이어야 합니다.",
                              ClientType::CUSTOMER);
            return;
        }

        if (!roomResult.ok) {
            ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                              Status::SERVER_ERROR, "채팅방 생성 실패",
                              ClientType::CUSTOMER);
            return;
        }

        // ── 2. 서브 스레드 감시 맵에 고객 등록 ───────────────
        //    이 시점부터 해당 roomId 로 enqueueMessage 가 오면
        //    워커 스레드가 이 fd 에 NTF_RECV_MSG 를 전송한다.
        ChatRoomManager::getInstance().joinRoom(
            roomResult.roomId,
            session->getFd(),
            ClientType::CUSTOMER);

        std::cout << "[CustomerChat] 채팅방 입장"
                  << " customerId=" << customerId
                  << " type=" << targetType
                  << " roomId=" << roomResult.roomId
                  << (roomResult.isNew ? " (신규생성)" : " (기존방)") << std::endl;

        // ── 3. 고객에게 응답 ──────────────────────────────────
        json res;
        res["status"]  = Status::SUCCESS;
        res["room_id"] = roomResult.roomId;
        res["is_new"]  = roomResult.isNew;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                            CmdChat::REQ_CREATE_ROOM, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[CustomerChat] handleChatCreateRoom 예외: "
                  << e.what() << std::endl;
        ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                          Status::SERVER_ERROR, "서버 오류",
                          ClientType::CUSTOMER);
    }
}

// ============================================================
//  handleChatSendMsg  (REQ_SEND_MSG = 601)
//
//  요청 JSON:
//    { "room_id": 7, "message": "안녕하세요" }
//
//  처리:
//    1. ChatDB::insertMessage   → DB 영속 저장
//    2. ChatRoomManager::enqueueMessage → 서브 스레드 큐 삽입
//       워커 스레드가 같은 방의 모든 참가자(고객 + 관리자/사장님)에게
//       NTF_RECV_MSG(604) Push
//
//  응답 JSON:
//    { "status": 2000, "message_id": 99, "sent_at": "14:32" }
// ============================================================
void CustomerHandler::handleChatSendMsg(Session* session,
                                         const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int  roomId = req.value("room_id", 0);
        std::string msg = req.value("message", "");

        if (roomId <= 0 || msg.empty()) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::BAD_REQUEST,
                              "room_id 또는 message 누락",
                              ClientType::CUSTOMER);
            return;
        }

        int senderId = getUserIdByFd(session->getFd());
        if (senderId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::UNAUTHORIZED, "로그인이 필요합니다.",
                              ClientType::CUSTOMER);
            return;
        }

        // ── 1. 채팅방 유효성 확인 ─────────────────────────────
        auto roomInfo = ChatDB::getInstance().queryRoom(roomId);
        if (!roomInfo.found) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::NOT_FOUND,
                              "존재하지 않는 채팅방입니다.",
                              ClientType::CUSTOMER);
            return;
        }

        // ── 2. DB 에 메시지 저장 ──────────────────────────────
        auto saveResult = ChatDB::getInstance()
                              .insertMessage(roomId, senderId, msg);
        if (!saveResult.ok) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::SERVER_ERROR, "메시지 저장 실패",
                              ClientType::CUSTOMER);
            return;
        }

        // ── 3. 고객에게 전송 성공 응답 ────────────────────────
        json res;
        res["status"]     = Status::SUCCESS;
        res["message_id"] = saveResult.messageId;
        res["sent_at"]    = saveResult.sentAt;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                            CmdChat::REQ_SEND_MSG, res.dump());

        // ── 4. 서브 스레드에 브로드캐스트 작업 등록 ──────────
        //    NTF_RECV_MSG(604) payload 구성
        json push;
        push["room_id"]   = roomId;
        push["sender_id"] = std::to_string(senderId);
        push["sender_role"] = "CUSTOMER";
        push["message"]   = msg;
        push["sent_at"]   = saveResult.sentAt;

        ChatRoomManager::getInstance().enqueueMessage(
            roomId,
            session->getFd(),
            CmdChat::NTF_RECV_MSG,
            push.dump());

        std::cout << "[CustomerChat] 메시지 전송"
                  << " senderId=" << senderId
                  << " roomId="   << roomId
                  << " len="      << msg.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[CustomerChat] handleChatSendMsg 예외: "
                  << e.what() << std::endl;
        ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                          Status::SERVER_ERROR, "서버 오류",
                          ClientType::CUSTOMER);
    }
}

// ============================================================
//  handleChatGetMsgs  (REQ_GET_MSGS = 602)
//
//  요청 JSON:
//    { "room_id": 7, "limit": 100 }   ← limit 생략 시 100 기본
//
//  응답 JSON:
//    {
//      "status": 2000,
//      "room_id": 7,
//      "messages": [
//        { "message_id":1, "sender_role":"CUSTOMER",
//          "content":"안녕하세요", "sent_at":"14:32" },
//        ...
//      ]
//    }
// ============================================================
void CustomerHandler::handleChatGetMsgs(Session* session,
                                         const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int  roomId = req.value("room_id", 0);
        int  limit  = req.value("limit", 100);

        if (roomId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS,
                              Status::BAD_REQUEST, "room_id 누락",
                              ClientType::CUSTOMER);
            return;
        }

        // 로그인 확인
        int customerId = getUserIdByFd(session->getFd());
        if (customerId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS,
                              Status::UNAUTHORIZED, "로그인이 필요합니다.",
                              ClientType::CUSTOMER);
            return;
        }

        // ── DB 에서 메시지 목록 조회 ──────────────────────────
        auto messages = ChatDB::getInstance().queryMessages(roomId, limit);

        json res;
        res["status"]   = Status::SUCCESS;
        res["room_id"]  = roomId;
        res["messages"] = json::array();

        for (const auto& m : messages) {
            json item;
            item["message_id"]  = m.messageId;
            item["sender_role"] = m.senderRole;  // "CUSTOMER" | "ADMIN" | "OWNER"
            item["content"]     = m.content;
            item["sent_at"]     = m.sentAt;
            res["messages"].push_back(item);
        }

        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                            CmdChat::REQ_GET_MSGS, res.dump());

        std::cout << "[CustomerChat] 메시지 조회"
                  << " customerId=" << customerId
                  << " roomId="     << roomId
                  << " count="      << messages.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[CustomerChat] handleChatGetMsgs 예외: "
                  << e.what() << std::endl;
        ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS,
                          Status::SERVER_ERROR, "서버 오류",
                          ClientType::CUSTOMER);
    }
}
