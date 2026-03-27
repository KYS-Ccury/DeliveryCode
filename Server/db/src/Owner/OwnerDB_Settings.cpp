#include "OwnerDB.h"
#include "Protocol.h"

using json = nlohmann::json;

json OwnerDB::getSettings(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    int ownerId = req.value("owner_id", -1);

    std::string query = "SELECT restaurant_name, phone, min_order_amt, base_delivery_fee, notice "
                        "FROM restaurants WHERE owner_id=" + std::to_string(ownerId) + " LIMIT 1";
    auto rows = db.executeQuery(query);

    if (!rows.empty()) {
        res["status"] = Status::SUCCESS;
        res["store_name"] = rows[0].at("restaurant_name");
        res["phone"] = rows[0].at("phone");
        res["min_order"] = std::stoi(rows[0].at("min_order_amt"));
        res["delivery_tip"] = std::stoi(rows[0].at("base_delivery_fee"));
        res["notice"] = rows[0].at("notice");
        // 시간 관련 컬럼이 있다면 여기서 같이 파싱해서 넣어줍니다.
    } else {
        res["status"] = Status::NOT_FOUND;
    }
    return res;
}

json OwnerDB::updateSettings(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    int ownerId = req.value("owner_id", -1);

    std::string name = req.value("store_name", "");
    std::string phone = req.value("phone", "");
    int minOrder = req.value("min_order", 0);
    int deliveryTip = req.value("delivery_tip", 0);
    std::string notice = req.value("notice", "");

    std::string query = "UPDATE restaurants SET "
                        "restaurant_name='" + MariaDBManager::escape(name) + "', "
                        "phone='" + MariaDBManager::escape(phone) + "', "
                        "min_order_amt=" + std::to_string(minOrder) + ", "
                        "base_delivery_fee=" + std::to_string(deliveryTip) + ", "
                        "notice='" + MariaDBManager::escape(notice) + "' "
                        "WHERE owner_id=" + std::to_string(ownerId);

    res["status"] = db.executeUpdate(query) ? Status::SUCCESS : Status::SERVER_ERROR;
    return res;
}