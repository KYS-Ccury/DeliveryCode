// #include "AdminDBHandler.h"
// #include <nlohmann/json.hpp>
// #include "MariaDBManager.h"
// #include "Protocol.h"
// #include <iostream>

// using json = nlohmann::json;

// // ============================================================
// // sendMsg — 관리자 메시지를 DB에 저장한다.
// // chat_messages 테이블에 INSERT한다.
// // ============================================================
// json AdminDBHandler::sendMsg(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 메시지 전송" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();
//         std::string roomId   = reqJson.value("room_id", "");
//         std::string message  = reqJson.value("message", "");
//         int         senderId = reqJson.value("sender_id", 0);

//         // 파라미터 검증을 수행한다.
//         if (roomId.empty() || message.empty()) {
//             json res;
//             res["status"]  = Status::BAD_REQUEST;
//             res["message"] = "room_id, message 필요";
//             return res;
//         }

//         // 메시지를 DB에 저장한다.
//         // SQL 인젝션 방지를 위해 이스케이프 처리한다.
//         auto escape = [](const std::string& s) -> std::string {
//             std::string out;
//             out.reserve(s.size() * 2);
//             for (char c : s) {
//                 if (c == '\'' || c == '\\' || c == '"') out += '\\';
//                 out += c;
//             }
//             return out;
//         };

//         std::string q =
//             "INSERT INTO chat_messages (room_id, sender_id, message, is_admin, created_at) "
//             "VALUES ('" + escape(roomId) + "', " + std::to_string(senderId) +
//             ", '" + escape(message) + "', 1, NOW())";

//         json res;
//         if (db.executeUpdate(q)) {
//             res["status"]  = Status::SUCCESS;
//             res["message"] = "전송 완료";
//         } else {
//             std::cout << "-------------------------" << std::endl;
//             std::cout << "관리자" << std::endl;
//             std::cout << "오류 : DB 메시지 저장 실패" << std::endl;
//             std::cout << "-------------------------" << std::endl;
//             res["status"]  = Status::SERVER_ERROR;
//             res["message"] = "메시지 저장 실패";
//         }
//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 메시지 전송 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }

// // ============================================================
// // getMsgs — 채팅 메시지를 DB에서 조회한다.
// // 지정된 room_id의 메시지를 시간순으로 반환한다.
// // ============================================================
// json AdminDBHandler::getMsgs(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 메시지 조회" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();
//         std::string roomId = reqJson.value("room_id", "");

//         // 파라미터 검증을 수행한다.
//         if (roomId.empty()) {
//             json res;
//             res["status"]  = Status::BAD_REQUEST;
//             res["message"] = "room_id 필요";
//             return res;
//         }

//         // 이스케이프 처리를 한다.
//         auto escape = [](const std::string& s) -> std::string {
//             std::string out;
//             out.reserve(s.size() * 2);
//             for (char c : s) {
//                 if (c == '\'' || c == '\\' || c == '"') out += '\\';
//                 out += c;
//             }
//             return out;
//         };

//         // 해당 채팅방의 메시지를 조회한다.
//         std::string q =
//             "SELECT message, is_admin, created_at "
//             "FROM chat_messages "
//             "WHERE room_id = '" + escape(roomId) + "' "
//             "ORDER BY created_at ASC "
//             "LIMIT 500";

//         DBResult rows = db.executeQuery(q);

//         // 결과 JSON을 구성한다.
//         json res;
//         res["status"]   = Status::SUCCESS;
//         res["messages"] = json::array();

//         for (auto& row : rows)
//         {
//             json item;
//             item["text"]     = row.count("message")    ? row.at("message")    : "";
//             item["is_admin"] = row.count("is_admin")   ? (row.at("is_admin") == "1") : false;
//             item["time"]     = row.count("created_at") ? row.at("created_at") : "";
//             res["messages"].push_back(item);
//         }
//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 메시지 조회 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }

// // ============================================================
// // roomList — 채팅방 목록을 DB에서 조회한다.
// // chat_rooms 테이블에서 마지막 메시지 포함하여 반환한다.
// // ============================================================
// json AdminDBHandler::roomList(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 채팅방 목록 조회" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();

//         // 채팅방 목록을 조회한다 (마지막 메시지 서브쿼리 포함).
//         std::string q =
//             "SELECT cr.room_id, "
//             "       IFNULL(u.login_id, '') AS user_name, "
//             "       IFNULL(u.role, '고객') AS role, "
//             "       ( SELECT cm.message FROM chat_messages cm "
//             "         WHERE cm.room_id = cr.room_id "
//             "         ORDER BY cm.created_at DESC LIMIT 1 "
//             "       ) AS last_message "
//             "FROM chat_rooms cr "
//             "LEFT JOIN users u ON cr.user_id = u.user_id "
//             "ORDER BY cr.room_id DESC "
//             "LIMIT 100";

//         DBResult rows = db.executeQuery(q);

//         // 결과 JSON을 구성한다.
//         json res;
//         res["status"] = Status::SUCCESS;
//         res["rooms"]  = json::array();

//         for (auto& row : rows)
//         {
//             json item;
//             item["room_id"]      = row.count("room_id")      ? row.at("room_id")      : "";
//             item["user_name"]    = row.count("user_name")     ? row.at("user_name")     : "";
//             item["role"]         = row.count("role")          ? row.at("role")           : "고객";
//             item["last_message"] = row.count("last_message")  ? row.at("last_message")  : "";
//             res["rooms"].push_back(item);
//         }
//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 채팅방 목록 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }