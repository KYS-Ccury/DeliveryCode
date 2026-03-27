// OwnerDB_Order.cpp
#include "OwnerDB.h"
#include "Protocol.h"

using json = nlohmann::json;

// 1. 주문 목록 조회
json OwnerDB::getOrderList(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    res["orders"] = json::array(); 

    // json에서 값 추출
    int ownerId = req.value("owner_id", -1);

    std::string restQuery = "SELECT restaurant_id FROM restaurants WHERE owner_id=" + std::to_string(ownerId) + " LIMIT 1";
    auto restRows = db.executeQuery(restQuery);
    
    if (restRows.empty()) {
        res["status"] = Status::NOT_FOUND;
        res["message"] = "등록된 매장이 없습니다.";
        return res;
    }
    
    std::string restaurantId = restRows[0].at("restaurant_id");

    std::string orderQuery = 
        "SELECT o.order_id, o.total_price, o.status, "
        "       (SELECT menu_name FROM order_items WHERE order_id = o.order_id LIMIT 1) as first_menu, "
        "       (SELECT COUNT(*) FROM order_items WHERE order_id = o.order_id) as menu_count "
        "FROM orders o "
        "WHERE o.restaurant_id = " + restaurantId + " "
        "  AND o.status IN ('PENDING', 'ACCEPTED', 'COOKING') "
        "ORDER BY o.created_at ASC"; 

    auto orderRows = db.executeQuery(orderQuery);

    for (const auto& row : orderRows) {
        json orderObj;
        orderObj["order_id"] = std::stoi(row.at("order_id"));
        orderObj["total_price"] = std::stoi(row.at("total_price"));
        orderObj["status"] = row.at("status");

        std::string menuName = row.at("first_menu");
        int menuCount = std::stoi(row.at("menu_count"));
        if (menuCount > 1) {
            menuName += " 외 " + std::to_string(menuCount - 1) + "건";
        }
        orderObj["menu_name"] = menuName;

        res["orders"].push_back(orderObj);
    }

    res["status"] = Status::SUCCESS;
    return res;
}

// 2. 주문 수락
json OwnerDB::acceptOrder(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;

    // json에서 값 추출
    int orderId = req.value("order_id", -1);
    int estMinutes = req.value("estimated_minutes", 15);

    std::string query = "UPDATE orders SET status='ACCEPTED', estimated_minutes=" +
                        std::to_string(estMinutes) +
                        " WHERE order_id=" + std::to_string(orderId) +
                        " AND status='PENDING'";

    res["status"] = db.executeUpdate(query) ? Status::SUCCESS : Status::SERVER_ERROR;
    return res;
}

// 3. 주문 거절
json OwnerDB::rejectOrder(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;

    // json에서 값 추출
    int orderId = req.value("order_id", -1);
    std::string reason = req.value("reason", "가게 사정으로 인한 취소");

    std::string query = "UPDATE orders SET status='REJECTED', cancel_reason='" +
                        MariaDBManager::escape(reason) +
                        "', canceled_at=NOW() WHERE order_id=" + std::to_string(orderId) +
                        " AND status='PENDING'";

    res["status"] = db.executeUpdate(query) ? Status::SUCCESS : Status::SERVER_ERROR;
    return res;
}