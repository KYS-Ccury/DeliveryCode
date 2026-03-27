#include "RiderDBHandler.h"
#include "Protocol.h"
#include "MariaDB_AcceptManager.h" // 변경된 DB 매니저 이름 적용
#include <iostream>

using json = nlohmann::json;

// ★ static 멤버 변수 정의 (헤더의 선언과 별개로 .cpp에 반드시 필요)
std::mutex RiderDBHandler::rider_db_mutex;

// ───────────────────────────────────────────────────────────────────
// ★ 단일 진입점: MiddleHandler에서 전달받은 프로토콜을 각 함수로 라우팅
// ───────────────────────────────────────────────────────────────────
json RiderDBHandler::process(uint16_t dbProtocol, const json& reqJson) {
    try {
        switch (dbProtocol) {
            // Auth/Profile (RiderDBHandler.cpp 내부)
            case CmdDBRider::REQ_DB_CREATE_PROFILE:    return createProfile(reqJson);
            case CmdDBRider::REQ_DB_LOGIN_HOOK:        return loginHook(reqJson);
            case CmdDBRider::REQ_DB_LOGOUT_HOOK:       return logoutHook(reqJson);
            case CmdDBRider::REQ_DB_GET_RIDER_PROFILE: return getProfile(reqJson);
            
            // Dispatch/Delivery (RiderDB_Delivery.cpp 내부)
            case CmdDBRider::REQ_DB_DISPATCH_LIST:     return getDispatchList(reqJson);
            case CmdDBRider::REQ_DB_ACCEPT_DISPATCH:   return acceptDispatch(reqJson);
            case CmdDBRider::REQ_DB_REJECT_DISPATCH:   return rejectDispatch(reqJson);
            case CmdDBRider::REQ_DB_PICKUP_DONE:       return pickupDone(reqJson);
            case CmdDBRider::REQ_DB_DELIVERY_DONE:     return deliveryDone(reqJson);
            case CmdDBRider::REQ_DB_MY_DISPATCHES:     return getMyDispatches(reqJson); 
            
            // Status/GPS (RiderDB_status.cpp 내부)
            case CmdDBRider::REQ_DB_WORK_STATUS:       return updateWorkStatus(reqJson);
            case CmdDBRider::REQ_DB_SEND_GPS:          return updateGps(reqJson);
            
            default:
                return {{"status", Status::SERVER_ERROR}, {"message", "알 수 없는 라이더 DB 프로토콜"}};
        }
    } catch (const std::exception& e) {
        std::cerr << "[RiderDBHandler] Exception: " << e.what() << std::endl;
        return {{"status", Status::SERVER_ERROR}, {"message", "DB 핸들러 내부 오류"}};
    }
}

// ───────────────────────────────────────────────────────────────────
// 계정 및 프로필 관련 구현부
// ───────────────────────────────────────────────────────────────────
json RiderDBHandler::createProfile(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    std::string loginId = db.escapeStr(reqJson.value("id", ""));
    
    std::string query = "SELECT user_id FROM users WHERE login_id='" + loginId + "' LIMIT 1";
    auto rows = db.executeQuery(query);
    
    if (rows.empty()) {
        res["status"] = Status::NOT_FOUND;
        res["message"] = "가입된 유저 ID를 찾을 수 없습니다.";
        return res;
    }

    int uid = std::stoi(rows[0].at("user_id"));
    std::string insertQuery = "INSERT INTO rider_profiles (user_id, vehicle_type, delivery_region, is_working, is_accepting) "
                              "VALUES (" + std::to_string(uid) + ", 'BIKE', '기본지역', FALSE, FALSE)";
    
    if (db.executeUpdate(insertQuery)) {
        res["status"] = Status::SUCCESS;
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "라이더 프로필 생성 실패";
    }
    return res;
}

json RiderDBHandler::loginHook(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int userId = reqJson.value("user_id", 0);

    std::string q = "SELECT u.name, u.phone, rp.vehicle_type, rp.is_working, rp.is_accepting "
                    "FROM users u JOIN rider_profiles rp ON rp.user_id = u.user_id "
                    "WHERE u.user_id = " + std::to_string(userId);
    
    auto rows = db.executeQuery(q);
    
    if (rows.empty()) {
        res["status"] = Status::NOT_FOUND;
        res["message"] = "라이더 프로필 정보를 찾을 수 없습니다.";
        return res;
    }

    db.executeUpdate("UPDATE rider_profiles SET is_online = TRUE WHERE user_id = " + std::to_string(userId));

    const auto& row = rows[0];
    res["status"]       = Status::SUCCESS;
    res["rider_id"]     = userId;
    res["name"]         = row.at("name");
    res["vehicle_type"] = row.at("vehicle_type");
    res["is_working"]   = (row.at("is_working") == "1");
    res["is_accepting"] = (row.at("is_accepting") == "1");
    return res;
}

json RiderDBHandler::logoutHook(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int userId = reqJson.value("user_id", 0);
    
    std::string query = "UPDATE rider_profiles SET is_online = FALSE, is_working = FALSE WHERE user_id = " + std::to_string(userId);
    
    if (db.executeUpdate(query)) {
        res["status"] = Status::SUCCESS;
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "로그아웃 상태 업데이트 실패";
    }
    return res;
}

json RiderDBHandler::getProfile(const json& reqJson) {
    std::lock_guard<std::mutex> lock(rider_db_mutex);
    json res;
    auto& db = MariaDB_AcceptManager::getInstance();
    int userId = reqJson.value("user_id", 0);

    if (reqJson.contains("vehicle_type")) {
        std::string vType = db.escapeStr(reqJson.value("vehicle_type", "BIKE"));
        db.executeUpdate("UPDATE rider_profiles SET vehicle_type = '" + vType + "' WHERE user_id = " + std::to_string(userId));
    }

    std::string query = "SELECT u.name, u.phone, rp.vehicle_type, rp.is_working "
                        "FROM users u JOIN rider_profiles rp ON rp.user_id = u.user_id "
                        "WHERE u.user_id = " + std::to_string(userId);
    
    auto rows = db.executeQuery(query);

    if (rows.empty()) {
        res["status"] = Status::NOT_FOUND;
        res["message"] = "사용자 없음";
        return res;
    }

    res["status"] = Status::SUCCESS;
    res["name"] = rows[0].at("name");
    res["phone"] = rows[0].at("phone");
    res["vehicle_type"] = rows[0].at("vehicle_type");
    res["is_working"] = (rows[0].at("is_working") == "1");
    return res;
}