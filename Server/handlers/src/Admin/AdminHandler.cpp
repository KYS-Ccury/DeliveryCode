#include "AdminHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

constexpr uint16_t CMD_ADMIN_GET_STATS = 4001;
constexpr uint16_t CMD_ADMIN_BAN_USER  = 4002;

void AdminHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    std::cout << "[AdminHandler] 관리자 명령어 수신: " << protocol << std::endl;

    try {
        json reqJson = jsonBody.empty() ? json{} : json::parse(jsonBody);
        json resJson;

        switch (protocol) {
            case CMD_ADMIN_GET_STATS: {
                // 서버 통계 요청 (예: 현재 접속자 수, 일일 주문량 등)
                // DB에서 통계 정보를 GROUP BY 로 긁어옴
                
                resJson["status"] = "SUCCESS";
                resJson["today_orders"] = 1503;
                resJson["active_riders"] = 124;
                break;
            }
            case CMD_ADMIN_BAN_USER: {
                std::string targetUserId = reqJson["target_user_id"];
                std::string reason = reqJson["reason"];
                
                // DB 연동: 블랙리스트 처리
                resJson["status"] = "SUCCESS";
                resJson["message"] = targetUserId + " 계정 정지 완료";
                break;
            }
            default:
                resJson["status"] = "ERROR";
                break;
        }

        // 응답 전송 (클라이언트 타입 4: 관리자)
        // session->sendPacket(4, protocol + 1, resJson.dump());

    } catch (const std::exception& e) {
        std::cerr << "[AdminHandler] 예외 발생: " << e.what() << std::endl;
    }
}



//////////// 마리아 DB 참고 예시 ////////////////////////
// #include "MariaDBManager.h"

// // 1. SELECT 예시 (고객 정보 조회)
// auto& db = MariaDBManager::getInstance();
// std::string query = "SELECT name, phone FROM users WHERE login_id = 'test_user'";
// DBResult rows = db.executeQuery(query);

// if (!rows.empty()) {
//     std::string userName = rows[0]["name"];
//     std::string userPhone = rows[0]["phone"];
// }

// // 2. UPDATE 예시 (사장님이 주문을 수락하여 상태 변경)
// std::string updateQuery = "UPDATE orders SET status = 'ACCEPTED' WHERE order_id = 123";
// bool success = db.executeUpdate(updateQuery);

// // 3. INSERT 예시 (새로운 주문 생성 후 PK 받아오기)
// std::string insertQuery = "INSERT INTO orders (customer_id, restaurant_id, total_price) VALUES (1, 10, 15000)";
// if (db.executeUpdate(insertQuery)) {
//     uint64_t newOrderId = db.getLastInsertId(); // 생성된 order_id (예: 124) 획득
// }