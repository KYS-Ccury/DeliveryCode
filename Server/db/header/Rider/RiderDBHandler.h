#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mutex>

class RiderDBHandler {
public:
    // MiddleHandler에서 호출할 1400번대 라우팅 전담 진입점
    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);

private:

    static std::mutex rider_db_mutex;

    // [RiderDBHandler.cpp] - 계정 및 프로필 관련 (Auth/Profile)
    static nlohmann::json createProfile(const nlohmann::json& reqJson);
    static nlohmann::json loginHook(const nlohmann::json& reqJson);
    static nlohmann::json logoutHook(const nlohmann::json& reqJson);
    static nlohmann::json getProfile(const nlohmann::json& reqJson);
    
    // [RiderDB_Delivery.cpp] - 배차 및 배달 관련 (Dispatch)
    static nlohmann::json getDispatchList(const nlohmann::json& reqJson);
    static nlohmann::json acceptDispatch(const nlohmann::json& reqJson);
    static nlohmann::json rejectDispatch(const nlohmann::json& reqJson);
    static nlohmann::json pickupDone(const nlohmann::json& reqJson);
    static nlohmann::json deliveryDone(const nlohmann::json& reqJson);
    static nlohmann::json getMyDispatches(const nlohmann::json& reqJson);
    
    // [RiderDB_status.cpp] - 상태 및 GPS 관련 (Status/GPS)
    static nlohmann::json updateWorkStatus(const nlohmann::json& reqJson);
    static nlohmann::json updateGps(const nlohmann::json& reqJson);
};