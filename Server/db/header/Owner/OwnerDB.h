#pragma once
#include "MariaDBManager.h"
#include <string>
#include <nlohmann/json.hpp>

class OwnerDB {
public:
    static OwnerDB& getInstance() {
        static OwnerDB inst;
        return inst;
    }
    
    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);
    
    bool insertOwnerProfile(int userId, const std::string& storeName);
    
    // 사장님 로그인 시 내 매장 정보 불러오기
    nlohmann::json getStoreInfoByOwnerId(int userId);

    nlohmann::json getOrderList(const nlohmann::json& req);
    nlohmann::json acceptOrder(const nlohmann::json& req);
    nlohmann::json rejectOrder(const nlohmann::json& req);

    nlohmann::json getMenuList(int ownerId);
    nlohmann::json addMenu(const nlohmann::json& req);
    nlohmann::json updateMenu(const nlohmann::json& req);
    nlohmann::json deleteMenu(const nlohmann::json& req);
    nlohmann::json getSalesStats(const nlohmann::json& req);
    nlohmann::json getSettings(const nlohmann::json& req);
    nlohmann::json updateSettings(const nlohmann::json& req);
    nlohmann::json updateStoreStatus(const nlohmann::json& req);

private:
    OwnerDB() = default;
};