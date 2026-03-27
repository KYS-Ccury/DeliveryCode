// ============================================================
//  RiderChatHandler.cpp  [라이더 채팅 핸들러]
//
//  ▶ 역할 분리 원칙
//    - Handler 계층 (이 파일) : 파라미터 검증, 흐름 제어
//    - ChatDB          계층  : 모든 SQL 쿼리 (영속 저장)
//    - ChatRoomManager 계층  : 실시간 브로드캐스트 (서브 스레드)
//
//  ▶ 채팅방 생성 시 동작
//    1. ChatDB::findOrCreateRoom   → DB 에 방 생성/조회
//    2. ChatRoomManager::joinRoom  → 서브 스레드가 감시하는 맵에 라이더 fd 등록
//       (관리자는 관리자 측 handleChatCreateRoom 에서 별도 joinRoom 호출)
//
//  ▶ 메시지 전송 시 동작
//    1. ChatDB::insertMessage           → DB 에 영속 저장
//    2. ChatRoomManager::enqueueMessage → 서브 스레드 큐에 삽입 (논블로킹)
//       서브 스레드가 같은 방 참가자 전원에게 NTF_RECV_MSG Push
//
//  ▶ 처리 프로토콜
//    CmdChat::REQ_CREATE_ROOM (600)
//    CmdChat::REQ_SEND_MSG    (601)
//    CmdChat::REQ_GET_MSGS    (602)
// ============================================================
#include "RiderHandler.h"
#include "ChatDB.h"
#include "ChatRoomManager.h"
#include "ChatUtil.h"
#include "Protocol.h"
#include "Session.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ============================================================
//  handleChatCreateRoom  (REQ_CREATE_ROOM = 600)
//
//  1. DB 에서 라이더 전용 채팅방 조회 or 생성
//  2. ChatRoomManager 의 방별 참가자 맵에 라이더 fd 등록
//     → 이 시점부터 서브 스레드가 해당 roomId 를 감시
// ============================================================
void RiderHandler::handleChatCreateRoom(Session* session, const std::string&) {
    try {
        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                              Status::UNAUTHORIZED, "로그인 필요", ClientType::RIDER);
            return;
        }

        // ── 1. DB 위임: 채팅방 조회 또는 신규 생성 ───────────
        // room_type = 'RIDER_ADMIN', key = riderId
        auto roomResult = ChatDB::getInstance()
                              .findOrCreateRoom(riderId, "RIDER_ADMIN");
        if (!roomResult.ok) {
            ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                              Status::SERVER_ERROR, "채팅방 생성 실패", ClientType::RIDER);
            return;
        }

        // ── 2. 서브 스레드 감시 맵에 라이더 등록 ─────────────
        //    이 시점부터 해당 roomId 로 enqueueMessage 가 오면
        //    워커 스레드가 이 fd 에 sendPacket 을 수행한다.
        ChatRoomManager::getInstance().joinRoom(
            roomResult.roomId, session->getFd(), ClientType::RIDER);

        // ── 3. 라이더에게 응답 ────────────────────────────────
        json res;
        res["status"]  = Status::SUCCESS;
        res["room_id"] = roomResult.roomId;
        res["is_new"]  = roomResult.isNew;
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdChat::REQ_CREATE_ROOM, res.dump());

        std::cout << "[RiderChat] 채팅방 입장 riderId=" << riderId
                  << " roomId=" << roomResult.roomId
                  << (roomResult.isNew ? " (신규생성)" : " (기존방)") << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[RiderChat] handleChatCreateRoom 예외: " << e.what() << std::endl;
        ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM,
                          Status::SERVER_ERROR, "서버 오류", ClientType::RIDER);
    }
}

// ============================================================
//  handleChatSendMsg  (REQ_SEND_MSG = 601)
//
//  1. ChatDB::insertMessage   → DB 에 영속 저장
//  2. ChatRoomManager::enqueueMessage → 서브 스레드 큐에 삽입
//     서브 스레드가 같은 방 참가자(라이더 + 관리자) 전원에게 Push
// ============================================================
void RiderHandler::handleChatSendMsg(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int  roomId = req.value("room_id", 0);
        std::string msg = req.value("message", "");

        if (roomId <= 0 || msg.empty()) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::BAD_REQUEST,
                              "room_id 또는 message 누락", ClientType::RIDER);
            return;
        }

        int senderId = getUserIdByFd(session->getFd());
        if (senderId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::UNAUTHORIZED, "로그인 필요", ClientType::RIDER);
            return;
        }

        // ── 1. DB 위임: 채팅방 유효성 확인 ───────────────────
        auto roomInfo = ChatDB::getInstance().queryRoom(roomId);
        if (!roomInfo.found) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::NOT_FOUND,
                              "존재하지 않는 채팅방입니다.", ClientType::RIDER);
            return;
        }

        // ── 2. DB 위임: 메시지 영속 저장 ─────────────────────
        auto msgResult = ChatDB::getInstance()
                             .insertMessage(roomId, senderId, msg);
        if (!msgResult.ok) {
            ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                              Status::SERVER_ERROR, "메시지 저장 실패", ClientType::RIDER);
            return;
        }

        // ── 3. 발신자(라이더)에게 전송 확인 응답 ─────────────
        json ack;
        ack["status"]     = Status::SUCCESS;
        ack["message_id"] = msgResult.messageId;
        ack["sent_at"]    = msgResult.sentAt;
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdChat::REQ_SEND_MSG, ack.dump());

        // ── 4. 서브 스레드 큐에 브로드캐스트 작업 등록 ────────
        //    ChatRoomManager 워커 스레드가 같은 roomId 참가자
        //    (라이더 fd + 관리자 fd) 전원에게 NTF_RECV_MSG 를 Push.
        //    이 함수는 즉시 반환(논블로킹).
        json push;
        push["room_id"]    = roomId;
        push["sender"]     = "RIDER";
        push["message"]    = msg;
        push["message_id"] = msgResult.messageId;
        push["sent_at"]    = msgResult.sentAt;

        ChatRoomManager::getInstance().enqueueMessage(
            roomId,
            session->getFd(),
            CmdChat::NTF_RECV_MSG,
            push.dump());

    } catch (const std::exception& e) {
        std::cerr << "[RiderChat] handleChatSendMsg 예외: " << e.what() << std::endl;
        ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG,
                          Status::SERVER_ERROR, "서버 오류", ClientType::RIDER);
    }
}

// ============================================================
//  handleChatGetMsgs  (REQ_GET_MSGS = 602)
//  DB 에서 해당 방의 메시지 목록을 조회해 반환한다.
//  (과거 메시지 로드 — 실시간 채널과 별개)
// ============================================================
void RiderHandler::handleChatGetMsgs(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int  roomId = req.value("room_id", 0);

        if (roomId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS,
                              Status::BAD_REQUEST, "room_id 누락", ClientType::RIDER);
            return;
        }

        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0) {
            ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS,
                              Status::UNAUTHORIZED, "로그인 필요", ClientType::RIDER);
            return;
        }

        // ── DB 위임: 메시지 목록 조회 ─────────────────────────
        auto messages = ChatDB::getInstance().queryMessages(roomId);

        json res;
        res["status"]   = Status::SUCCESS;
        res["room_id"]  = roomId;
        res["messages"] = json::array();
        for (const auto& m : messages) {
            json item;
            item["message_id"] = m.messageId;
            item["sender"]     = m.senderRole;
            item["message"]    = m.content;
            item["sent_at"]    = m.sentAt;
            res["messages"].push_back(item);
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdChat::REQ_GET_MSGS, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[RiderChat] handleChatGetMsgs 예외: " << e.what() << std::endl;
        ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS,
                          Status::SERVER_ERROR, "서버 오류", ClientType::RIDER);
    }
}
