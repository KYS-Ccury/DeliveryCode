// OwnerDB_Auth.cpp
#include "OwnerDB.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

bool OwnerDB::insertOwnerProfile(const nlohmann::json& req) {
    auto& db = MariaDBManager::getInstance();
    
    int ownerId = req.value("owner_id", -1);
    std::string storeName = req.value("store_name", "");
    std::string phone = req.value("phone", "");
    std::string address = req.value("address", "");

    // 🚨 [핵심 수정] DB 필수값(NOT NULL)인 category_id=1, biz_no='000-00-00000' 추가!
    std::string query = "INSERT INTO restaurants (owner_id, category_id, restaurant_name, biz_no, phone, address) VALUES (" +
                        std::to_string(ownerId) + ", 1, '" +
                        MariaDBManager::escape(storeName) + "', '000-00-00000', '" +
                        MariaDBManager::escape(phone) + "', '" +
                        MariaDBManager::escape(address) + "')";

    if (db.executeUpdate(query)) {
        std::cout << "[OwnerDB] " << ownerId << "번 사장님의 매장(" << storeName << ") 생성 성공!\n";
        return true;
    } else {
        std::cerr << "[OwnerDB] 매장 생성 실패! (DB 제약조건 또는 쿼리 오류)\n";
        return false;
    }
}

json OwnerDB::getStoreInfoByOwnerId(int userId) {
    auto& db = MariaDBManager::getInstance();
    json storeInfo = json::object(); 

    // 🚨 [핵심 수정] 테이블 이름을 stores 가 아니라 restaurants 로 변경!
    std::string query = "SELECT restaurant_id, restaurant_name, is_open FROM restaurants WHERE owner_id=" + std::to_string(userId) + " LIMIT 1";
    auto rows = db.executeQuery(query);

    if (!rows.empty()) {
        storeInfo["status"] = Status::SUCCESS; 
        storeInfo["store_id"] = std::stoi(rows[0].at("restaurant_id"));
        storeInfo["store_name"] = rows[0].at("restaurant_name");
        storeInfo["store_status"] = std::stoi(rows[0].at("is_open"));
    } else {
        storeInfo["status"] = Status::NOT_FOUND;
    }
    return storeInfo;
}