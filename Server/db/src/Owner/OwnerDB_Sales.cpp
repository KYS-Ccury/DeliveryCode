// OwnerDB_Sales.cpp
#include "OwnerDB.h"
#include "Protocol.h"

using json = nlohmann::json;

json OwnerDB::getSalesStats(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    res["sales_list"] = json::array();
    
    int ownerId = req.value("owner_id", -1);
    std::string startDate = req.value("start_date", "");
    std::string endDate = req.value("end_date", "");

    // 1. 내 매장 ID 찾기
    auto restRows = db.executeQuery("SELECT restaurant_id FROM restaurants WHERE owner_id=" + std::to_string(ownerId) + " LIMIT 1");
    if (restRows.empty()) {
        res["status"] = Status::NOT_FOUND;
        return res;
    }
    std::string restId = restRows[0].at("restaurant_id");

    // 2. 매출 내역 조회 쿼리 ('DONE' 상태인 완료된 주문만 계산)
    // 날짜 포맷 주의 (시작일 00:00:00 ~ 종료일 23:59:59)
    std::string query = 
        "SELECT o.order_id, DATE_FORMAT(o.created_at, '%Y-%m-%d %H:%i') as order_time, o.total_price, "
        "       (SELECT menu_name FROM order_items WHERE order_id = o.order_id LIMIT 1) as first_menu, "
        "       (SELECT COUNT(*) FROM order_items WHERE order_id = o.order_id) as menu_count "
        "FROM orders o "
        "WHERE o.restaurant_id = " + restId + " "
        "  AND o.status = 'DONE' "
        "  AND o.created_at >= '" + MariaDBManager::escape(startDate) + " 00:00:00' "
        "  AND o.created_at <= '" + MariaDBManager::escape(endDate) + " 23:59:59' "
        "ORDER BY o.created_at DESC";

    auto rows = db.executeQuery(query);
    int totalRevenue = 0; // 총 매출액 합산용

    for (const auto& row : rows) {
        json item;
        item["order_id"] = std::stoi(row.at("order_id"));
        item["time"] = row.at("order_time");
        
        int price = std::stoi(row.at("total_price"));
        item["price"] = price;
        totalRevenue += price; // 합산

        std::string menuName = row.at("first_menu");
        int menuCount = std::stoi(row.at("menu_count"));
        if (menuCount > 1) menuName += " 외 " + std::to_string(menuCount - 1) + "건";
        item["menu_name"] = menuName;

        res["sales_list"].push_back(item);
    }

    res["total_revenue"] = totalRevenue;
    res["total_count"] = (int)rows.size();
    res["status"] = Status::SUCCESS;
    
    return res;
}