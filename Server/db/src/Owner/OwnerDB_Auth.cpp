// OwnerDB_Auth.cpp
#include "OwnerDB.h"
#include "Protocol.h"

using json = nlohmann::json;

bool OwnerDB::insertOwnerProfile(int userId, const std::string& storeName) {
    auto& db = MariaDBManager::getInstance();
    
    std::string query = "INSERT INTO stores (owner_id, store_name, status) VALUES (" 
                      + std::to_string(userId) + ", '" 
                      + MariaDBManager::escape(storeName) + "', 'CLOSED')";
                      
    return db.executeUpdate(query);
}

json OwnerDB::getStoreInfoByOwnerId(int userId) {
    auto& db = MariaDBManager::getInstance();
    json storeInfo = json::object(); 

    std::string query = "SELECT store_id, store_name, status FROM stores WHERE owner_id=" + std::to_string(userId) + " LIMIT 1";
    auto rows = db.executeQuery(query);

    if (!rows.empty()) {
        storeInfo["status"] = Status::SUCCESS; 
        storeInfo["store_id"] = std::stoi(rows[0].at("store_id"));
        storeInfo["store_name"] = rows[0].at("store_name");
        storeInfo["store_status"] = rows[0].at("status");
    } else {
        storeInfo["status"] = Status::NOT_FOUND;
    }
    return storeInfo;
}