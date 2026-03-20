#include "OwnerHandler.h"
#include "Session.h"
#include "Packet.h"          // CmdOwner, ClientType, Status 포함
#include "MariaDBManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include "Types.h"

using json = nlohmann::json;

void OwnerHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    std::cout << "[OwnerHandler] 사장님 요청 수신 - 프로토콜: " << protocol << std::endl;

    switch (protocol) {
        case CmdOwner::REQ_ACCEPT_ORDER: // 주문 수락 (305)
            handleAcceptOrder(session, jsonBody);
            break;

        case CmdOwner::REQ_UPDATE_MENU:  // 품절 처리를 메뉴 수정(303)으로 매핑
            handleSoldOut(session, jsonBody);
            break;

        default:
            std::cerr << "[Owner] 알 수 없는 사장님 프로토콜: " << protocol << std::endl;
            // 에러 응답 로직 추가 가능
            break;
    }
}

// ---------------------------------------------------------
// [기능 구현] 1. 주문 수락 (기능 개발 중)
// ---------------------------------------------------------
void OwnerHandler::handleAcceptOrder(Session* session, const std::string& jsonBody) {
    try {
        json req = jsonBody.empty() ? json{} : json::parse(jsonBody);
        // auto& db = MariaDBManager::getInstance();

        // value()를 사용하면 키가 없을 때의 기본값을 지정할 수 있어 안전합니다.
        std::string orderId = req.value("order_id", "");
        int estimatedTime = req.value("estimated_time", 0); // 예상 조리 시간

        // DB 연동 (개발 예정): 
        // std::string updateQuery = "UPDATE orders SET status='COOKING' WHERE id=" + orderId;
        // db.executeUpdate(updateQuery);

        json res;
        res["status"] = Status::SUCCESS;
        res["message"] = "주문 접수 완료";

        // 응답 전송 (클라이언트 타입 2: 사장님)
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), 
                            CmdOwner::REQ_ACCEPT_ORDER, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleAcceptOrder] JSON 예외 발생: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------
// [기능 구현] 2. 메뉴 품절 처리 (기능 개발 중)
// ---------------------------------------------------------
void OwnerHandler::handleSoldOut(Session* session, const std::string& jsonBody) {
    try {
        json req = jsonBody.empty() ? json{} : json::parse(jsonBody);
        // auto& db = MariaDBManager::getInstance();

        std::string menuId = req.value("menu_id", "");

        // DB 연동 (개발 예정): 해당 메뉴 상태를 품절로 업데이트
        // db.executeUpdate("UPDATE menus SET is_soldout=1 WHERE id=" + menuId);

        json res;
        res["status"] = Status::SUCCESS;
        res["message"] = "품절 처리 완료";

        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), 
                            CmdOwner::REQ_UPDATE_MENU, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleSoldOut] JSON 예외 발생: " << e.what() << std::endl;
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