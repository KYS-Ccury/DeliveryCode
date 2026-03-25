#include "RiderHandler.h"
#include "Session.h"
#include "Struct.h"
#include "Protocol.h"
#include "MariaDBManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

// ================================================================
//  handleWorkStatus  (REQ_WORK_STATUS = 406)
//
//  클라이언트 action 값:
//    "ONLINE"       → 운행 시작 (is_working=TRUE, is_accepting=TRUE)
//    "OFFLINE"      → 운행 종료 (is_working=FALSE, is_accepting=FALSE)
//    "DISPATCH_ON"  → 배차 수락 ON
//    "DISPATCH_OFF" → 배차 수락 OFF
//    "VEHICLE"      → 차량 변경 (vehicle_type 필드 포함)
//                     ※ CMD_RIDER_STATUS_UPDATE(406)로 차량 변경 시 여기서 처리
// ================================================================
void RiderHandler::handleWorkStatus(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "");

        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0 || action.empty()) {
            sendError(session, CmdRider::REQ_WORK_STATUS,
                      Status::BAD_REQUEST, "파라미터 오류 또는 비로그인");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        std::string updateQ;

        if (action == "ONLINE") {
            updateQ = "UPDATE rider_profiles "
                      "SET is_working=TRUE, is_accepting=TRUE "
                      "WHERE user_id=" + std::to_string(riderId);

        } else if (action == "OFFLINE") {
            updateQ = "UPDATE rider_profiles "
                      "SET is_working=FALSE, is_accepting=FALSE "
                      "WHERE user_id=" + std::to_string(riderId);

        } else if (action == "DISPATCH_ON") {
            updateQ = "UPDATE rider_profiles "
                      "SET is_accepting=TRUE "
                      "WHERE user_id=" + std::to_string(riderId);

        } else if (action == "DISPATCH_OFF") {
            updateQ = "UPDATE rider_profiles "
                      "SET is_accepting=FALSE "
                      "WHERE user_id=" + std::to_string(riderId);

        } else if (action == "VEHICLE") {
            // 차량 종류 변경
            std::string vt = escapeStr(req.value("vehicle_type", "BIKE"));
            updateQ = "UPDATE rider_profiles "
                      "SET vehicle_type='" + vt +
                      "' WHERE user_id=" + std::to_string(riderId);
        } else {
            sendError(session, CmdRider::REQ_WORK_STATUS,
                      Status::BAD_REQUEST, "알 수 없는 action: " + action);
            return;
        }

        if (db.executeUpdate(updateQ)) {
            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = action;
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdRider::REQ_WORK_STATUS, res.dump());
        } else {
            sendError(session, CmdRider::REQ_WORK_STATUS,
                      Status::SERVER_ERROR, "DB 업데이트 실패");
        }

    } catch (const std::exception& e) {
        std::cerr << "[handleWorkStatus] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_WORK_STATUS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleUpdateGps  (REQ_SEND_GPS = 407)
//  { "latitude": 35.123, "longitude": 126.456 }
// ================================================================
void RiderHandler::handleUpdateGps(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        double lat = req.value("latitude",  0.0);
        double lng = req.value("longitude", 0.0);
        int riderId = getUserIdByFd(session->getFd());

        if (riderId <= 0 || (lat == 0.0 && lng == 0.0)) return;

        auto& db = MariaDBManager::getInstance();
        std::ostringstream q;
        q << "UPDATE users u "
          << "JOIN rider_profiles rp ON u.user_id = rp.user_id "
          << "SET u.latitude=" << lat << ", u.longitude=" << lng
          << ", rp.last_location_at=NOW() "
          << "WHERE u.user_id=" << riderId;

        db.executeUpdate(q.str());

    } catch (const std::exception& e) {
        std::cerr << "[handleUpdateGps] 예외: " << e.what() << std::endl;
    }
}
