#include "RiderDBHandler.h"
#include "Protocol.h"
#include "MariaDB_AcceptManager.h"
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

json RiderDBHandler::getDispatchList(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    
    std::string q =
        "SELECT o.order_id, r.restaurant_name AS store_name, r.address AS pickup_addr, o.delivery_address AS dest_addr, "
        "r.base_delivery_fee AS delivery_fee, o.total_price, TIMESTAMPDIFF(SECOND, o.created_at, NOW()) AS elapsed_sec "
        "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
        "WHERE o.status = 'READY' AND o.rider_id IS NULL AND o.order_type = 'DELIVERY' ORDER BY o.created_at ASC"; 

    DBResult rows = db.executeQuery(q);
    
    res["status"] = Status::SUCCESS;
    res["orders"] = json::array();

    for (const auto& row : rows) {
        json item;
        item["order_id"]     = std::stoi(row.at("order_id")); 
        item["store_name"]   = row.at("store_name");
        item["pickup_addr"]  = row.at("pickup_addr"); 
        item["dest_addr"]    = row.at("dest_addr");
        item["delivery_fee"] = std::stoi(row.at("delivery_fee")); 
        item["total_price"]  = std::stoi(row.at("total_price"));
        item["elapsed_sec"]  = std::stoi(row.at("elapsed_sec"));
        res["orders"].push_back(item);
    }
    return res;
}

json RiderDBHandler::acceptDispatch(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    int orderId = reqJson.value("order_id", 0);
    
    db.executeUpdate("START TRANSACTION");

    std::string checkQ = "SELECT order_id FROM orders WHERE order_id = " + std::to_string(orderId) + 
                         " AND status = 'READY' AND rider_id IS NULL FOR UPDATE";
    DBResult check = db.executeQuery(checkQ);
    
    if (check.empty()) {
        db.executeUpdate("ROLLBACK");
        res["status"] = Status::NOT_FOUND;
        res["message"] = "이미 다른 라이더가 배차를 수락했거나 유효하지 않은 주문입니다.";
        return res;
    }

    if (!db.executeUpdate("UPDATE orders SET rider_id = " + std::to_string(riderId) + ", status = 'DISPATCHED' WHERE order_id = " + std::to_string(orderId))) {
        db.executeUpdate("ROLLBACK");
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "DB 업데이트 실패";
        return res;
    }

    db.executeUpdate("INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES (" + std::to_string(orderId) + ", " + std::to_string(riderId) + ", 'ACCEPTED')");
    db.executeUpdate("INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES (" + std::to_string(orderId) + ", 'READY', 'DISPATCHED', " + std::to_string(riderId) + ")");
    db.executeUpdate("COMMIT");

    std::string detailQ = "SELECT o.order_id, r.restaurant_name AS store_name, r.address AS pickup_addr, "
                          "o.delivery_address AS dest_addr, r.phone AS store_phone, r.base_delivery_fee AS delivery_fee, o.total_price "
                          "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id WHERE o.order_id = " + std::to_string(orderId);
    DBResult detail = db.executeQuery(detailQ);

    res["status"] = Status::SUCCESS; 
    res["order_id"] = orderId;
    
    if (!detail.empty()) {
        const DBRow& d = detail[0];
        std::ostringstream code; 
        code << "ORD" << std::setw(6) << std::setfill('0') << orderId;
        res["order_code"]   = code.str(); 
        res["store_name"]   = d.at("store_name");
        res["pickup_addr"]  = d.at("pickup_addr"); 
        res["store_phone"]  = d.at("store_phone");
        res["dest_addr"]    = d.at("dest_addr"); 
        res["delivery_fee"] = std::stoi(d.at("delivery_fee"));
        res["total_price"]  = std::stoi(d.at("total_price"));
    }
    return res;
}

json RiderDBHandler::rejectDispatch(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    int orderId = reqJson.value("order_id", 0);
    std::string reason = db.escapeStr(reqJson.value("reason", "MANUAL"));

    db.executeUpdate("INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES (" + 
                     std::to_string(orderId) + ", " + std::to_string(riderId) + ", '" + reason + "')");
    
    return {{"status", Status::SUCCESS}};
}

json RiderDBHandler::pickupDone(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    int orderId = reqJson.value("order_id", 0);
    
    bool ok = db.executeUpdate("UPDATE orders SET status = 'DELIVERING' WHERE order_id = " + 
                               std::to_string(orderId) + " AND rider_id = " + std::to_string(riderId) + " AND status = 'DISPATCHED'");
    
    if (!ok) {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "상태 업데이트 실패";
        return res;
    }

    db.executeUpdate("INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES (" + 
                     std::to_string(orderId) + ", 'DISPATCHED', 'DELIVERING', " + std::to_string(riderId) + ")");

    res["status"] = Status::SUCCESS; 
    res["message"] = "픽업 완료, 배달을 시작합니다.";
    return res;
}

json RiderDBHandler::deliveryDone(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    int orderId = reqJson.value("order_id", 0);

    db.executeUpdate("START TRANSACTION");

    DBResult check = db.executeQuery("SELECT o.order_id, r.base_delivery_fee AS delivery_fee FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id WHERE o.order_id = " + 
                                     std::to_string(orderId) + " AND o.rider_id = " + std::to_string(riderId) + " AND o.status = 'DELIVERING' FOR UPDATE");
    if (check.empty()) {
        db.executeUpdate("ROLLBACK");
        res["status"] = Status::NOT_FOUND;
        res["message"] = "유효하지 않은 주문이거나 이미 완료된 주문입니다.";
        return res;
    }

    int deliveryFee = std::stoi(check[0].at("delivery_fee"));
    
    db.executeUpdate("UPDATE orders SET status = 'DELIVERED', is_masked = TRUE WHERE order_id = " + std::to_string(orderId));
    db.executeUpdate("INSERT INTO rider_earnings (rider_id, order_id, delivery_fee) VALUES (" + std::to_string(riderId) + ", " + std::to_string(orderId) + ", " + std::to_string(deliveryFee) + ")");
    db.executeUpdate("INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES (" + std::to_string(orderId) + ", 'DELIVERING', 'DELIVERED', " + std::to_string(riderId) + ")");
    db.executeUpdate("COMMIT");

    res["status"] = Status::SUCCESS; 
    res["delivery_fee"] = deliveryFee; 
    res["message"] = "배달이 완료되었습니다.";
    return res;
}

json RiderDBHandler::getMyDispatches(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    bool summaryOnly = reqJson.value("summary_only", false);

    if (summaryOnly) {
        DBResult rows = db.executeQuery("SELECT COUNT(*) AS cnt, IFNULL(SUM(re.delivery_fee),0) AS total FROM orders o JOIN rider_earnings re ON re.order_id = o.order_id WHERE o.rider_id = " + 
                                        std::to_string(riderId) + " AND DATE(o.created_at) = CURDATE() AND o.status = 'DELIVERED'");
        res["status"] = Status::SUCCESS; 
        res["today_count"] = rows.empty() ? 0 : std::stoi(rows[0].at("cnt")); 
        res["today_fee"] = rows.empty() ? 0 : std::stoi(rows[0].at("total"));
        return res;
    }

    std::string q = "SELECT o.order_id, r.restaurant_name AS store_name, o.status, o.total_price, re.delivery_fee, DATE_FORMAT(o.created_at, '%m/%d %H:%i') AS created_at "
                    "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id LEFT JOIN rider_earnings re ON re.order_id = o.order_id AND re.rider_id = " + 
                    std::to_string(riderId) + " WHERE o.rider_id = " + std::to_string(riderId) + " AND o.status = 'DELIVERED' ORDER BY o.created_at DESC LIMIT 50";
    DBResult rows = db.executeQuery(q);

    res["status"] = Status::SUCCESS; 
    res["records"] = json::array();
    
    for (const auto& row : rows) {
        json item;
        int oid = std::stoi(row.at("order_id"));
        std::ostringstream code; code << "ORD" << std::setw(6) << std::setfill('0') << oid;
        
        item["order_id"] = oid; 
        item["order_code"] = code.str(); 
        item["store_name"] = row.at("store_name");
        item["delivery_fee"] = row.at("delivery_fee").empty() ? 0 : std::stoi(row.at("delivery_fee"));
        item["status"] = row.at("status"); 
        item["created_at"] = row.at("created_at");
        res["records"].push_back(item);
    }
    return res;
}