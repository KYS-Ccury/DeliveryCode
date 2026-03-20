#include "CustomerHandler.h"
#include "MariaDBManager.h"
// nlohmann/json 같은 JSON 라이브러리 사용 권장
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    // 1. 가변 길이 Body 파싱 (JSON)
    json parsedData = json::parse(jsonBody);

    // 2. 2바이트 프로토콜(명령어)에 따른 처리
    if (protocol == CMD_CUSTOMER_ORDER) {
        std::string menuId = parsedData["menu_id"];
        int count = parsedData["count"];

        // 3. DB 처리
        auto& db = MariaDBManager::getInstance();
        bool success = db.query("INSERT INTO orders ...");

        // 4. 처리 결과 응답 생성 및 전송
        json responseJson;
        responseJson["status"] = success ? "OK" : "FAIL";
        std::string responseStr = responseJson.dump();

        // 7바이트 응답 헤더 생성 후 Session을 통해 Send
        session->sendPacket(1, CMD_CUSTOMER_ORDER_RES, responseStr);
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