#include "RiderHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

constexpr uint16_t CMD_RIDER_UPDATE_GPS = 3001;
constexpr uint16_t CMD_RIDER_PICKUP     = 3002;

void RiderHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    try {
        json reqJson = jsonBody.empty() ? json{} : json::parse(jsonBody);
        json resJson;

        switch (protocol) {
            case CMD_RIDER_UPDATE_GPS: {
                // 초당 수없이 들어오는 GPS 데이터 (가장 가벼워야 함)
                std::string riderId = reqJson["rider_id"];
                double lat = reqJson["latitude"];
                double lng = reqJson["longitude"];
                
                // DB Update 또는 메모리 캐시(Redis 등)에 위치 업데이트
                // GPS 갱신은 굳이 매번 응답 패킷을 주지 않고 넘길 수도 있습니다.
                return; // 응답 없이 종료
            }
            case CMD_RIDER_PICKUP: {
                std::string orderId = reqJson["order_id"];
                
                // DB 연동: 픽업 완료 처리
                resJson["status"] = "SUCCESS";
                resJson["message"] = "픽업 확인, 배달을 시작합니다.";
                break;
            }
            default:
                resJson["status"] = "ERROR";
                break;
        }

        // 응답 전송 (클라이언트 타입 3: 라이더)
        // session->sendPacket(3, protocol + 1, resJson.dump());

    } catch (const std::exception& e) {
        std::cerr << "[RiderHandler] 예외 발생: " << e.what() << std::endl;
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