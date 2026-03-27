// // ============================================================
// //  AdminChatHandler.cpp [관리자 전담 핸들러]
// // ============================================================
// #include "AdminChatHandler.h"
// #include "ChatUtil.h"
// #include "EpollServer.h"
// #include "AdminHandler.h"
// #include "RiderHandler.h"
// #include "Session.h"
// #include "MariaDBManager.h"
// #include "Protocol.h"
// #include <nlohmann/json.hpp>
// #include <iostream>

// using json = nlohmann::json;

// AdminChatHandler& AdminChatHandler::getInstance() {
//     static AdminChatHandler instance;
//     return instance;
// }

// void AdminChatHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
//     switch (protocol) {
//         case CmdChat::REQ_CREATE_ROOM: handleCreateRoom(session, jsonBody); break;
//         case CmdChat::REQ_SEND_MSG:    handleSendMsg(session, jsonBody); break;
//         case CmdChat::REQ_GET_MSGS:    handleGetMsgs(session, jsonBody); break;
//         default: break;
//     }
// }

// void AdminChatHandler::handleCreateRoom(Session* session, const std::string& jsonBody) {
//     // 관리자가 특정 라이더와 강제로 채팅방을 개설할 때 사용 가능
//     try {
//         json req = jsonBody.empty() ? json::object() : json::parse(jsonBody);
//         int riderId = req.value("rider_id", 0);

//         if (riderId <= 0) {
//             ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::BAD_REQUEST, "대상 라이더 ID 누락", ClientType::ADMIN);
//             return;
//         }

//         auto& db = MariaDBManager::getInstance();
//         std::string checkQ =
//             "SELECT room_id FROM chat_rooms WHERE order_id = " + std::to_string(riderId) + " AND room_type = 'RIDER_ADMIN' AND is_active = TRUE LIMIT 1";
//         DBResult rows = db.executeQuery(checkQ);

//         int roomId = 0; bool isNew = false;
//         if (!rows.empty()) {
//             roomId = std::stoi(rows[0].at("room_id"));
//         } else {
//             db.executeUpdate("INSERT INTO chat_rooms (order_id, room_type, is_active) VALUES (" + std::to_string(riderId) + ", 'RIDER_ADMIN', TRUE)");
//             roomId = static_cast<int>(db.getLastInsertId());
//             isNew  = true;
//         }

//         json res; res["status"] = Status::SUCCESS; res["room_id"] = roomId; res["is_new"] = isNew;
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdChat::REQ_CREATE_ROOM, res.dump());
//     } catch (const std::exception& e) {
//         ChatUtil::sendErr(session, CmdChat::REQ_CREATE_ROOM, Status::SERVER_ERROR, "서버 오류", ClientType::ADMIN);
//     }
// }

// void AdminChatHandler::handleSendMsg(Session* session, const std::string& jsonBody) {
//     try {
//         json req = json::parse(jsonBody);
//         int roomId = req.value("room_id", 0);
//         std::string msg = req.value("message", "");

//         if (roomId <= 0 || msg.empty()) {
//             ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::BAD_REQUEST, "파라미터 누락", ClientType::ADMIN);
//             return;
//         }

//         int senderId = AdminHandler::getInstance().getUserIdByFd(session->getFd());
//         if (senderId <= 0) {
//             ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::UNAUTHORIZED, "로그인 필요", ClientType::ADMIN);
//             return;
//         }

//         auto& db = MariaDBManager::getInstance();
//         DBResult room = db.executeQuery(
//             "SELECT room_id, order_id, room_type FROM chat_rooms WHERE room_id = " + std::to_string(roomId) + " AND is_active = TRUE LIMIT 1");
        
//         if (room.empty()) {
//             ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::NOT_FOUND, "채팅방 없음", ClientType::ADMIN);
//             return;
//         }

//         db.executeUpdate("INSERT INTO chat_messages (room_id, sender_id, content) VALUES (" + std::to_string(roomId) + ", " + std::to_string(senderId) + ", '" + ChatUtil::escStr(msg) + "')");
        
//         int msgId = static_cast<int>(db.getLastInsertId());
//         DBResult ts = db.executeQuery("SELECT DATE_FORMAT(sent_at, '%H:%i') AS t FROM chat_messages WHERE message_id = " + std::to_string(msgId));
//         std::string sentAt = ts.empty() ? "" : ts[0].at("t");

//         json res; res["status"] = Status::SUCCESS; res["message_id"] = msgId; res["sent_at"] = sentAt;
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdChat::REQ_SEND_MSG, res.dump());

//         // 대상 판별하여 Push 전송 (RIDER_ADMIN 방인 경우 라이더에게 전송)
//         std::string roomType = room[0].at("room_type");
//         int targetId = std::stoi(room[0].at("order_id")); 
        
//         json push; push["room_id"] = roomId; push["sender"] = "ADMIN"; push["message"] = msg; push["sent_at"] = sentAt;

//         int targetFd = 0;
//         ClientType targetType = ClientType::RIDER;

//         if (roomType == "RIDER_ADMIN") {
//             targetFd = RiderHandler::getInstance().getFdByUserId(targetId);
//             targetType = ClientType::RIDER;
//         }

//         if (targetFd > 0 && EpollServer::s_instance) {
//             auto targetSess = EpollServer::s_instance->getSession(targetFd);
//             if (targetSess) {
//                 targetSess->sendPacket(static_cast<uint8_t>(targetType), CmdChat::NTF_RECV_MSG, push.dump());
//             }
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "[AdminChat] handleSendMsg 예외: " << e.what() << std::endl;
//         ChatUtil::sendErr(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "서버 오류", ClientType::ADMIN);
//     }
// }

// void AdminChatHandler::handleGetMsgs(Session* session, const std::string& jsonBody) {
//     try {
//         json req = json::parse(jsonBody);
//         int roomId = req.value("room_id", 0);
//         if (roomId <= 0) return ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS, Status::BAD_REQUEST, "room_id 누락", ClientType::ADMIN);

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
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdChat::REQ_GET_MSGS, res.dump());
//     } catch (const std::exception& e) {
//         ChatUtil::sendErr(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "서버 오류", ClientType::ADMIN);
//     }
// }