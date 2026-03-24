#include "RiderHandler.h"
#include "MariaDBManager.h"

using json = nlohmann::json;

void RiderHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
    auto& db = MariaDBManager::getInstance();

    // 1. 라이더 전용 로직: 프로필 조회 및 온라인 상태 변경
    std::string q = "SELECT u.name, u.phone, rp.vehicle_type, rp.is_working, rp.is_accepting "
                    "FROM users u JOIN rider_profiles rp ON rp.user_id = u.user_id "
                    "WHERE u.user_id = " + std::to_string(userId);
    
    DBResult rows = db.executeQuery(q);
    const DBRow& row = rows[0];

    db.executeUpdate("UPDATE rider_profiles SET is_online = TRUE WHERE user_id = " + std::to_string(userId));

    // 2. 라이더 전용 응답 패킷 전송
    json res;
    res["status"]       = Status::SUCCESS;
    res["rider_id"]     = userId;
    res["name"]         = row.at("name");
    res["vehicle_type"] = row.at("vehicle_type");
    res["is_working"]   = (row.at("is_working") == "1");
    res["is_accepting"] = (row.at("is_accepting") == "1");

    session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCommon::REQ_LOGIN, res.dump());
}