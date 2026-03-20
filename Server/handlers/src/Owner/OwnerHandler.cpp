#include "OwnerHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 가상의 사장님용 프로토콜 상수
constexpr uint16_t CMD_OWNER_ACCEPT_ORDER = 2001;
constexpr uint16_t CMD_OWNER_SOLD_OUT    = 2002;

void OwnerHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    std::cout << "[OwnerHandler] 사장님 요청 수신 - 프로토콜: " << protocol << std::endl;

    try {
        json reqJson = jsonBody.empty() ? json{} : json::parse(jsonBody);
        json resJson;
        // MariaDBManager& db = MariaDBManager::getInstance();

        switch (protocol) {
            case CMD_OWNER_ACCEPT_ORDER: {
                std::string orderId = reqJson["order_id"];
                int estimatedTime = reqJson["estimated_time"]; // 예상 조리 시간
                
                // DB 연동: 주문 상태를 '조리 중'으로 업데이트
                // db.query("UPDATE orders SET status='COOKING' WHERE id=" + orderId);
                
                resJson["status"] = "SUCCESS";
                resJson["message"] = "주문 접수 완료";
                break;
            }
            case CMD_OWNER_SOLD_OUT: {
                std::string menuId = reqJson["menu_id"];
                
                // DB 연동: 해당 메뉴 품절 처리
                resJson["status"] = "SUCCESS";
                resJson["message"] = "품절 처리 완료";
                break;
            }
            default:
                resJson["status"] = "ERROR";
                resJson["message"] = "알 수 없는 사장님 프로토콜";
                break;
        }

        // 응답 전송 (클라이언트 타입 2: 사장님)
        // session->sendPacket(2, protocol + 1, resJson.dump());

    } catch (const std::exception& e) {
        std::cerr << "[OwnerHandler] 예외 발생: " << e.what() << std::endl;
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