// Owner_Chat.cpp
#include "OwnerHandler.h"
#include "MiddleHandler.h"
#include "ChatRoomManager.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// 1. 채팅방 접속 (방 생성/조회 및 메모리 등록) - 600번
void OwnerHandler::handleCreateRoom(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        
        // 1) DB 처리: 방 찾기 또는 생성 (1600번)
        // 클라이언트가 "room_type":"CUSTOMER_OWNER", "key":주문번호 를 보낸다고 가정
        json dbRes = MiddleHandler::processDBRequest(CmdDBChat::REQ_DB_FIND_OR_CREATE_ROOM, req);

        if (dbRes["status"] == Status::SUCCESS) {
            int roomId = dbRes.value("room_id", 0);
            
            // 2) 메모리 처리: 사장님 소켓을 해당 방에 조인!
            ChatRoomManager::getInstance().joinRoom(roomId, session->getFd(), ClientType::OWNER);
            std::cout << "[Owner_Chat] 사장님(fd:" << session->getFd() << ") " << roomId << "번 방 입장 완료.\n";
            
            // 3) (옵션) 입장과 동시에 이전 대화 내역도 같이 뽑아서 주고 싶다면 1603번 호출
            json msgReq = { {"room_id", roomId}, {"limit", 50} };
            json msgRes = MiddleHandler::processDBRequest(CmdDBChat::REQ_DB_QUERY_MESSAGES, msgReq);
            if (msgRes["status"] == Status::SUCCESS) {
                dbRes["messages"] = msgRes["messages"]; // 응답에 대화 내역 끼워넣기
            }
        }
        
        // 클라이언트에게 방 입장 완료(및 과거 내역) 응답 전송
        sendResponse(session, CmdChat::REQ_CREATE_ROOM, dbRes);
    } catch (...) {
        sendError(session, CmdChat::REQ_CREATE_ROOM, Status::SERVER_ERROR, "방 입장 실패");
    }
}

// 2. 메시지 전송 (DB 저장 후 브로드캐스트) - 601번
void OwnerHandler::handleSendMsg(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        int roomId = req.value("room_id", 0);
        
        // 1) DB 처리: 메시지 INSERT (1602번)
        json dbRes = MiddleHandler::processDBRequest(CmdDBChat::REQ_DB_INSERT_MESSAGE, req);

        if (dbRes["status"] == Status::SUCCESS) {
            // 2) 메모리 브로드캐스트: 같은 방 사람들에게 "알림(NTF_RECV_MSG: 604)" 형태로 뿌리기!
            
            json ntfPayload;
            ntfPayload["room_id"] = roomId;
            ntfPayload["sender_id"] = req.value("sender_id", 0);
            ntfPayload["sender_role"] = "OWNER";
            ntfPayload["content"] = req.value("content", "");
            ntfPayload["sent_at"] = dbRes.value("sent_at", ""); // DB에서 찍힌 시간

            ChatRoomManager::getInstance().enqueueMessage(
                roomId, session->getFd(), CmdChat::NTF_RECV_MSG, ntfPayload.dump()
            );
        }
        
        // 3) 보낸 본인에게는 "전송 성공" 이라는 601 응답 전송
        sendResponse(session, CmdChat::REQ_SEND_MSG, dbRes);
    } catch (...) {
        sendError(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "메시지 전송 실패");
    }
}

// 3. 메시지 과거 내역 단독 조회 (필요 시 호출) - 602번
void OwnerHandler::handleGetMsgs(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        json dbRes = MiddleHandler::processDBRequest(CmdDBChat::REQ_DB_QUERY_MESSAGES, req);
        sendResponse(session, CmdChat::REQ_GET_MSGS, dbRes);
    } catch (...) {
        sendError(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "메시지 조회 실패");
    }
}