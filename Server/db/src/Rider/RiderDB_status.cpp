#include "RiderDBHandler.h"
#include "Protocol.h"
#include "MariaDB_AcceptManager.h"
#include <sstream>

using json = nlohmann::json;

json RiderDBHandler::updateWorkStatus(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    std::string action = reqJson.value("action", "");
    std::string updateQ;

    if (action == "ONLINE") {
        updateQ = "UPDATE rider_profiles SET is_working = TRUE, is_accepting = TRUE WHERE user_id = " + std::to_string(riderId);
    } else if (action == "OFFLINE") {
        updateQ = "UPDATE rider_profiles SET is_working = FALSE, is_accepting = FALSE WHERE user_id = " + std::to_string(riderId);
    } else if (action == "DISPATCH_ON") {
        updateQ = "UPDATE rider_profiles SET is_accepting = TRUE WHERE user_id = " + std::to_string(riderId);
    } else if (action == "DISPATCH_OFF") {
        updateQ = "UPDATE rider_profiles SET is_accepting = FALSE WHERE user_id = " + std::to_string(riderId);
    } else {
        return {{"status", Status::BAD_REQUEST}, {"message", "알 수 없는 action"}};
    }

    if (db.executeUpdate(updateQ)) {
        res["status"] = Status::SUCCESS;
        res["action"] = action;
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "DB 업데이트 실패";
    }
    return res;
}

json RiderDBHandler::updateGps(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    auto& db = MariaDB_AcceptManager::getInstance();
    int riderId = reqJson.value("rider_id", 0);
    double lat = reqJson.value("latitude", 0.0);
    double lng = reqJson.value("longitude", 0.0);
    
    std::ostringstream q;
    q << "UPDATE users u JOIN rider_profiles rp ON u.user_id = rp.user_id "
      << "SET u.latitude = " << lat << ", u.longitude = " << lng << ", rp.last_location_at = NOW() "
      << "WHERE u.user_id = " << riderId;
      
    db.executeUpdate(q.str());
    
    return {{"status", Status::SUCCESS}}; 
}