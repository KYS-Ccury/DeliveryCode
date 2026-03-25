// // ============================================================
// //  OwnerChatHandler.cpp [사장 전담 핸들러]
// // ============================================================
// #include "OwnerChatHandler.h"
// #include "ChatUtil.h"
// #include "EpollServer.h"
// #include "OwnerHandler.h"
// #include "CustomerHandler.h"
// #include "Session.h"
// #include "MariaDBManager.h"
// #include "Protocol.h"
// #include <nlohmann/json.hpp>
// #include <iostream>

// using json = nlohmann::json;

// OwnerChatHandler& OwnerChatHandler::getInstance() {
//     static OwnerChatHandler instance;
//     return instance;
// }

// void OwnerChatHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
//     switch (protocol) {
//         case CmdChat::REQ_CREATE_ROOM: handleCreateRoom(session, jsonBody); break;
//         case CmdChat::REQ_SEND_MSG:    handleSendMsg(session, jsonBody); break;
//         case CmdChat::REQ_GET_MSGS:    handleGetMsgs(session, jsonBody); break;
//         default: break;
//     }
// }

// void OwnerChatHandler::handleCreateRoom(Session* session, const std::string& jsonBody) {
//     // 사장님은 일반적으로 고객이 만든 방에 참여하므로 에러를 반환하거나 방 조회 기능만 제공합니다.
//     ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::BAD_REQUEST, "방은 고객이 생성합니다.", ClientType::OWNER);
// }

// void OwnerChatHandler::handleSendMsg(Session* session, const std::string& jsonBody) {
//     try {
//         json req = json::parse(jsonBody);
//         int roomId = req.value("room_id", 0);
//         std::string msg = req.value("message", "");

//         if (roomId <= 0 || msg.empty()) {
//             ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::BAD_REQUEST, "파라미터 누락", ClientType::OWNER);
//             return;
//         }

//         int senderId = OwnerHandler::getInstance().getUserIdByFd(session->getFd());
//         if (senderId <= 0) {
//             ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::UNAUTHORIZED, "로그인 필요", ClientType::OWNER);
//             return;
//         }

//         auto& db = MariaDBManager::getInstance();
//         DBResult room = db.executeQuery(
//             "SELECT room_id, order_id FROM chat_rooms WHERE room_id = " + std::to_string(roomId) + " AND is_active = TRUE LIMIT 1");
        
//         if (room.empty()) {
//             ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::NOT_FOUND, "채팅방 없음", ClientType::OWNER);
//             return;
//         }

//         db.executeUpdate("INSERT INTO chat_messages (room_id, sender_id, content) VALUES (" + std::to_string(roomId) + ", " + std::to_string(senderId) + ", '" + ChatUtil::escStr(msg) + "')");
        
//         int msgId = static_cast<int>(db.getLastInsertId());
//         DBResult ts = db.executeQuery("SELECT DATE_FORMAT(sent_at, '%H:%i') AS t FROM chat_messages WHERE message_id = " + std::to_string(msgId));
//         std::string sentAt = ts.empty() ? "" : ts[0].at("t");

//         json res;
//         res["status"]     = Status::SUCCESS;
//         res["message_id"] = msgId;
//         res["sent_at"]    = sentAt;
//         session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdChat::REQ_SEND_MSG, res.dump());

//         // 고객에게 Push 알림 전송 (order_id 컬럼에 고객 ID가 저장되어 있다고 가정)
//         json push;
//         push["room_id"]  = roomId;
//         push["sender"]   = "OWNER";
//         push["message"]  = msg;
//         push["sent_at"]  = sentAt;
        
//         int targetCustomerId = std::stoi(room[0].at("order_id")); 
//         int targetFd = CustomerHandler::getInstance().getFdByUserId(targetCustomerId);
        
//         if (targetFd > 0 && EpollServer::s_instance) {
//             auto targetSess = EpollServer::s_instance->getSession(targetFd);
//             if (targetSess) {
//                 targetSess->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdChat::NTF_RECV_MSG, push.dump());
//             }
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "[OwnerChat] handleSendMsg 예외: " << e.what() << std::endl;
//         ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "서버 오류", ClientType::OWNER);
//     }
// }

// void OwnerChatHandler::handleGetMsgs(Session* session, const std::string& jsonBody) {
//     try {
//         json req = json::parse(jsonBody);
//         int roomId = req.value("room_id", 0);
//         if (roomId <= 0) return ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS, Status::BAD_REQUEST, "room_id 누락", ClientType::OWNER);

//         auto& db = MariaDBManager::getInstance();
//         std::string q =
//             "SELECT cm.message_id, u.role AS sender_role, cm.content, DATE_FORMAT(cm.sent_at, '%H:%i') AS sent_at "
//             "FROM chat_messages cm JOIN users u ON u.user_id = cm.sender_id "
//             "WHERE cm.room_id = " + std::to_string(roomId) + " ORDER BY cm.sent_at ASC LIMIT 100";
//         DBResult rows = db.executeQuery(q);

//         json res; res["status"] = Status::SUCCESS; res["messages"] = json::array();
//         for (const auto& row : rows) {
//             json item; item["sender"] = row.at("sender_role"); item["message"] = row.at("content"); item["sent_at"] = row.at("sent_at");
//             res["messages"].push_back(item);
//         }
//         session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdChat::REQ_GET_MSGS, res.dump());
//     } catch (const std::exception& e) {
//         ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "서버 오류", ClientType::OWNER);
//     }
// }