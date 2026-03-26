#include "RiderDB.h"
#include "CommonDB.h"    // 비밀번호 확인/변경은 CommonDB 사용
#include <iostream>
#include <sstream>
#include <iomanip>

// ── 내부 유틸 ──────────────────────────────────────────────
static std::string makeOrderCode(int orderId) {
    std::ostringstream ss;
    ss << "ORD" << std::setw(6) << std::setfill('0') << orderId;
    return ss.str();
}

// ============================================================
//  [라이더 프로필]
// ============================================================
bool RiderDB::insertRiderProfile(int userId, const std::string& vehicleType) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "INSERT INTO rider_profiles "
        "(user_id, vehicle_type, is_working, is_accepting, is_online) "
        "VALUES (" + std::to_string(userId) +
        ",'" + CommonDB::escape(vehicleType) + "',FALSE,FALSE,FALSE)");
}

bool RiderDB::insertRiderProfileIfMissing(int userId) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM rider_profiles "
        "WHERE user_id=" + std::to_string(userId) + " LIMIT 1");
    if (!rows.empty()) return true;
    return insertRiderProfile(userId, "BIKE");
}

RiderDB::RiderProfile RiderDB::queryRiderProfile(int userId) {
    RiderProfile result;
    auto& db = MariaDBManager::getInstance();
    // users JOIN rider_profiles — users 정보는 CommonDB.queryUserBasic 과
    // 중복되지만 라이더는 rider_profiles 와 함께 한 번에 조회해야 효율적
    auto rows = db.executeQuery(
        "SELECT u.name, u.phone, "
        "       rp.vehicle_type, rp.is_working, rp.is_accepting "
        "FROM users u "
        "LEFT JOIN rider_profiles rp ON rp.user_id = u.user_id "
        "WHERE u.user_id=" + std::to_string(userId) + " LIMIT 1");

    if (rows.empty()) return result;
    const auto& r  = rows[0];
    result.found       = true;
    result.name        = r.count("name")         ? r.at("name")         : "";
    result.phone       = r.count("phone")        ? r.at("phone")        : "";
    result.vehicleType = r.count("vehicle_type") && !r.at("vehicle_type").empty()
                         ? r.at("vehicle_type") : "BIKE";
    result.isWorking   = r.count("is_working")   && r.at("is_working")   == "1";
    result.isAccepting = r.count("is_accepting") && r.at("is_accepting") == "1";
    return result;
}

bool RiderDB::setOnline(int userId, bool online) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE rider_profiles SET is_online=" +
        std::string(online ? "TRUE" : "FALSE") +
        " WHERE user_id=" + std::to_string(userId));
}

bool RiderDB::changeVehicleType(int userId, const std::string& vehicleType) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE rider_profiles SET vehicle_type='" + CommonDB::escape(vehicleType) +
        "' WHERE user_id=" + std::to_string(userId));
}

bool RiderDB::changeAccountInfo(int userId,
                                const std::string& bankName,
                                const std::string& accountHolder,
                                const std::string& accountNumber) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE rider_profiles SET "
        "bank_name='"      + CommonDB::escape(bankName)       + "', "
        "account_holder='" + CommonDB::escape(accountHolder)  + "', "
        "account_number='" + CommonDB::escape(accountNumber)  + "' "
        "WHERE user_id="   + std::to_string(userId));
}

// ============================================================
//  [배차 / 배달]
// ============================================================
std::vector<RiderDB::DispatchOrder> RiderDB::queryDispatchList() {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT o.order_id, r.restaurant_name AS store_name, "
        "       r.address AS pickup_addr, o.delivery_address AS dest_addr, "
        "       r.base_delivery_fee AS delivery_fee, o.total_price, "
        "       TIMESTAMPDIFF(SECOND, o.created_at, NOW()) AS elapsed_sec "
        "FROM orders o "
        "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
        "WHERE o.status='ACCEPTED' AND o.rider_id IS NULL "
        "  AND o.delivery_method='DELIVERY' "
        "ORDER BY o.created_at ASC");

    std::vector<DispatchOrder> result;
    for (const auto& row : rows) {
        DispatchOrder item;
        item.orderId     = std::stoi(row.at("order_id"));
        item.storeName   = row.at("store_name");
        item.pickupAddr  = row.at("pickup_addr");
        item.destAddr    = row.at("dest_addr");
        item.deliveryFee = row.count("delivery_fee") && !row.at("delivery_fee").empty()
                           ? std::stoi(row.at("delivery_fee")) : 0;
        item.totalPrice  = row.count("total_price")  && !row.at("total_price").empty()
                           ? std::stoi(row.at("total_price"))  : 0;
        item.elapsedSec  = row.count("elapsed_sec")  && !row.at("elapsed_sec").empty()
                           ? std::stoi(row.at("elapsed_sec"))  : 0;
        result.push_back(item);
    }
    return result;
}

RiderDB::AcceptResult RiderDB::acceptDispatch(int orderId, int riderId) {
    AcceptResult result;
    auto& db = MariaDBManager::getInstance();

    bool txOk = db.executeTransaction([&]() -> bool {
        // 아직 배차 안 된 ACCEPTED 주문인지 확인
        auto check = db.executeQuery(
            "SELECT order_id FROM orders "
            "WHERE order_id=" + std::to_string(orderId) +
            "  AND status='ACCEPTED' AND rider_id IS NULL LIMIT 1");
        if (check.empty()) return false;

        // 라이더 배정 + 상태 DELIVERING
        bool ok = db.executeUpdate(
            "UPDATE orders SET rider_id=" + std::to_string(riderId) +
            ", status='DELIVERING' "
            "WHERE order_id=" + std::to_string(orderId));
        if (!ok) return false;

        // 배차 이력 기록
        db.executeUpdate(
            "INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES ("
            + std::to_string(orderId) + ","
            + std::to_string(riderId) + ",'ACCEPT')");

        // 주문 상태 이력 기록
        db.executeUpdate(
            "INSERT INTO order_status_logs "
            "(order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderId) + ",'ACCEPTED','DELIVERING',"
            + std::to_string(riderId) + ")");

        return true;
    });

    if (!txOk) return result;   // result.ok == false

    // 주문 상세 조회 (배차 수락 응답용)
    auto detail = db.executeQuery(
        "SELECT o.order_id, r.restaurant_name AS store_name, "
        "       r.address AS pickup_addr, o.delivery_address AS dest_addr, "
        "       r.phone AS store_phone, r.base_delivery_fee AS delivery_fee, "
        "       o.total_price, o.customer_id "
        "FROM orders o "
        "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
        "WHERE o.order_id=" + std::to_string(orderId) + " LIMIT 1");

    result.ok = true;
    if (!detail.empty()) {
        const auto& d         = detail[0];
        result.detail.found       = true;
        result.detail.orderId     = orderId;
        result.detail.storeName   = d.at("store_name");
        result.detail.pickupAddr  = d.at("pickup_addr");
        result.detail.storePhone  = d.count("store_phone") ? d.at("store_phone") : "";
        result.detail.destAddr    = d.at("dest_addr");
        result.detail.deliveryFee = d.count("delivery_fee") && !d.at("delivery_fee").empty()
                                    ? std::stoi(d.at("delivery_fee")) : 0;
        result.detail.totalPrice  = d.count("total_price")  && !d.at("total_price").empty()
                                    ? std::stoi(d.at("total_price"))  : 0;
        result.detail.customerId  = d.count("customer_id")  && !d.at("customer_id").empty()
                                    ? std::stoi(d.at("customer_id"))  : 0;
    }
    return result;
}

bool RiderDB::rejectDispatch(int orderId, int riderId, const std::string& reason) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES ("
        + std::to_string(orderId) + ","
        + std::to_string(riderId) + ",'"
        + CommonDB::escape(reason) + "')");
}

bool RiderDB::pickupDone(int orderId, int riderId) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "INSERT INTO order_status_logs "
        "(order_id, from_status, to_status, changed_by) VALUES ("
        + std::to_string(orderId) +
        ",'DELIVERING','DELIVERING'," + std::to_string(riderId) + ")");
}

RiderDB::DeliveryDoneResult RiderDB::deliveryDone(int orderId, int riderId) {
    DeliveryDoneResult result;
    auto& db = MariaDBManager::getInstance();

    bool txOk = db.executeTransaction([&]() -> bool {
        auto check = db.executeQuery(
            "SELECT r.base_delivery_fee AS fee, o.customer_id "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderId) +
            "  AND o.rider_id=" + std::to_string(riderId) +
            "  AND o.status='DELIVERING' LIMIT 1");

        if (check.empty()) return false;

        result.deliveryFee = check[0].count("fee") && !check[0].at("fee").empty()
                             ? std::stoi(check[0].at("fee")) : 0;
        result.customerId  = check[0].count("customer_id") && !check[0].at("customer_id").empty()
                             ? std::stoi(check[0].at("customer_id")) : 0;

        // 주문 완료 + 개인정보 마스킹
        db.executeUpdate(
            "UPDATE orders SET status='DONE', is_masked=TRUE "
            "WHERE order_id=" + std::to_string(orderId));

        // 수익 기록
        db.executeUpdate(
            "INSERT INTO rider_earnings (rider_id, order_id, delivery_fee) VALUES ("
            + std::to_string(riderId) + ","
            + std::to_string(orderId) + ","
            + std::to_string(result.deliveryFee) + ")");

        // 상태 이력 기록
        db.executeUpdate(
            "INSERT INTO order_status_logs "
            "(order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderId) +
            ",'DELIVERING','DONE'," + std::to_string(riderId) + ")");

        return true;
    });

    result.ok = txOk;
    return result;
}

RiderDB::MyDispatchSummary RiderDB::queryTodaySummary(int riderId) {
    MyDispatchSummary result;
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT COUNT(*) AS cnt, "
        "       IFNULL(SUM(re.delivery_fee), 0) AS total "
        "FROM orders o "
        "JOIN rider_earnings re ON re.order_id = o.order_id "
        "WHERE o.rider_id=" + std::to_string(riderId) +
        "  AND DATE(o.created_at)=CURDATE() AND o.status='DONE'");

    if (!rows.empty()) {
        result.todayCount = rows[0].count("cnt")   && !rows[0].at("cnt").empty()
                            ? std::stoi(rows[0].at("cnt"))   : 0;
        result.todayFee   = rows[0].count("total") && !rows[0].at("total").empty()
                            ? std::stoi(rows[0].at("total")) : 0;
    }
    return result;
}

std::vector<RiderDB::MyDispatchRecord> RiderDB::queryMyDispatches(int riderId) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT o.order_id, r.restaurant_name AS store_name, o.status, "
        "       IFNULL(re.delivery_fee,0) AS delivery_fee, "
        "       DATE_FORMAT(o.created_at,'%m/%d %H:%i') AS created_at "
        "FROM orders o "
        "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
        "LEFT JOIN rider_earnings re "
        "  ON re.order_id=o.order_id AND re.rider_id=" + std::to_string(riderId) +
        " WHERE o.rider_id=" + std::to_string(riderId) +
        "   AND o.status='DONE' "
        "ORDER BY o.created_at DESC LIMIT 50");

    std::vector<MyDispatchRecord> result;
    for (const auto& row : rows) {
        MyDispatchRecord rec;
        rec.orderId     = std::stoi(row.at("order_id"));
        rec.orderCode   = makeOrderCode(rec.orderId);
        rec.storeName   = row.at("store_name");
        rec.deliveryFee = row.count("delivery_fee") && !row.at("delivery_fee").empty()
                          ? std::stoi(row.at("delivery_fee")) : 0;
        rec.status      = row.at("status");
        rec.createdAt   = row.count("created_at") ? row.at("created_at") : "";
        result.push_back(rec);
    }
    return result;
}

// ============================================================
//  [상태 / GPS]
// ============================================================
bool RiderDB::setWorkStatus(int riderId,
                            const std::string& action,
                            const std::string& vehicleType) {
    auto& db = MariaDBManager::getInstance();
    std::string q;

    if (action == "ONLINE") {
        q = "UPDATE rider_profiles "
            "SET is_working=TRUE, is_accepting=TRUE "
            "WHERE user_id=" + std::to_string(riderId);

    } else if (action == "OFFLINE") {
        q = "UPDATE rider_profiles "
            "SET is_working=FALSE, is_accepting=FALSE "
            "WHERE user_id=" + std::to_string(riderId);

    } else if (action == "DISPATCH_ON") {
        q = "UPDATE rider_profiles "
            "SET is_accepting=TRUE "
            "WHERE user_id=" + std::to_string(riderId);

    } else if (action == "DISPATCH_OFF") {
        q = "UPDATE rider_profiles "
            "SET is_accepting=FALSE "
            "WHERE user_id=" + std::to_string(riderId);

    } else if (action == "VEHICLE") {
        q = "UPDATE rider_profiles "
            "SET vehicle_type='" + CommonDB::escape(vehicleType) +
            "' WHERE user_id=" + std::to_string(riderId);

    } else {
        std::cerr << "[RiderDB] setWorkStatus: 알 수 없는 action=" << action << std::endl;
        return false;
    }

    return db.executeUpdate(q);
}

bool RiderDB::updateGps(int riderId, double lat, double lng) {
    auto& db = MariaDBManager::getInstance();
    std::ostringstream q;
    q << "UPDATE users u "
      << "JOIN rider_profiles rp ON u.user_id = rp.user_id "
      << "SET u.latitude="  << lat
      << ", u.longitude="   << lng
      << ", rp.last_location_at=NOW() "
      << "WHERE u.user_id=" << riderId;
    return db.executeUpdate(q.str());
}
