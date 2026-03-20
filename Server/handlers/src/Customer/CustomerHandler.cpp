#include "CustomerHandler.h"
#include "Session.h"
#include "Packet.h"          // CmdCustomer, Status 네임스페이스 포함
#include "MariaDBManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include "Types.h"

using json = nlohmann::json;

void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    // 1차적으로 로그 출력 (디버깅 용도)
    std::cout << "[CustomerHandler] Protocol: " << protocol << " 처리 시작" << std::endl;

    switch (protocol) {
        case CmdCustomer::REQ_STORE_LIST:
            handleStoreList(session, jsonBody);
            break;

        case CmdCustomer::REQ_CREATE_ORDER:
            handleCreateOrder(session, jsonBody);
            break;

        case CmdCustomer::REQ_ORDER_HISTORY:
            handleOrderHistory(session, jsonBody);
            break;

        default:
            std::cerr << "[Customer] 알 수 없는 프로토콜: " << protocol << std::endl;
            // 필요 시 클라이언트에게 에러 프로토콜 전송
            break;
    }
}



///////////////////////////참고만 하고 따로 파일 분리시켜서 만들기 바람...........

// ---------------------------------------------------------
// [기능 구현] 1. 매장 목록 조회
// ---------------------------------------------------------
void CustomerHandler::handleStoreList(Session* session, const std::string& jsonBody) {
    auto& db = MariaDBManager::getInstance();
    
    // 예시 SQL: 실제 테이블 구조에 맞게 수정 필요
    // DBResult rows = db.executeQuery("SELECT id, name, category FROM stores");
    
    json res;
    res["status"] = Status::SUCCESS;
    res["stores"] = json::array(); // 여기에 DB 결과 루프 돌며 push_back
    
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), 
                        CmdCustomer::REQ_STORE_LIST, res.dump());
}

// ---------------------------------------------------------
// [기능 구현] 2. 주문 생성
// ---------------------------------------------------------
void CustomerHandler::handleCreateOrder(Session* session, const std::string& jsonBody) {
    try {
        json reqData = json::parse(jsonBody);
        auto& db = MariaDBManager::getInstance();

        // 비즈니스 로직 (예: INSERT INTO orders...)
        // bool success = db.executeUpdate("INSERT ...");

        json res;
        res["status"] = Status::SUCCESS;
        res["order_id"] = 12345; // db.getLastInsertId() 등 활용

        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), 
                            CmdCustomer::REQ_CREATE_ORDER, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleCreateOrder] JSON Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------
// [기능 구현] 3. 주문 내역 조회
// ---------------------------------------------------------
void CustomerHandler::handleOrderHistory(Session* session, const std::string& jsonBody) {
    // 주문 내역 로직 구현...
    std::cout << "주문 내역 조회 로직 실행" << std::endl;
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