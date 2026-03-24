// ============================================================
//  RiderHandler_Status.cpp
//  라이더 상태(출퇴근/수락상태) 및 GPS 위치 전송 구현부
// ============================================================
#include "RiderHandler.h"
#include "Session.h"
#include "Struct.h"
#include "Protocol.h"
#include "MariaDBManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

static void sendError(Session* session, uint16_t protocol, uint16_t statusCode, const std::string& message) {
    json res; res["status"] = statusCode; res["message"] = message;
    session->sendPacket(static_cast<uint8_t>(ClientType::RIDER), protocol, res.dump());
}

void RiderHandler::handleWorkStatus(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "");
        
        // 부모(BaseHandler)의 함수를 사용하여 ID 조회
        int riderId = getUserIdByFd(session->getFd());

        if (riderId <= 0 || action.empty()) { 
            // 부모의 m_clientType 멤버 변수를 사용하면 좋습니다.
            sendError(session, CmdRider::REQ_WORK_STATUS, Status::BAD_REQUEST, "파라미터 오류"); 
            return; 
        }

        auto& db = MariaDBManager::getInstance(); // DB 매니저 인스턴스 확보
        std::string updateQ; // 변수 선언 추가

        if (action == "ONLINE") {
            updateQ = "UPDATE rider_profiles SET is_working = TRUE, is_accepting = TRUE WHERE user_id = " + std::to_string(riderId);
        } else if (action == "OFFLINE") {
            updateQ = "UPDATE rider_profiles SET is_working = FALSE, is_accepting = FALSE WHERE user_id = " + std::to_string(riderId);
        } else if (action == "DISPATCH_ON") {
            updateQ = "UPDATE rider_profiles SET is_accepting = TRUE WHERE user_id = " + std::to_string(riderId);
        } else if (action == "DISPATCH_OFF") {
            updateQ = "UPDATE rider_profiles SET is_accepting = FALSE WHERE user_id = " + std::to_string(riderId);
        } else {
            sendError(session, CmdRider::REQ_WORK_STATUS, Status::BAD_REQUEST, "알 수 없는 action: " + action);
            return;
        }

        if (db.executeUpdate(updateQ)) {
            json res;
            res["status"] = Status::SUCCESS;
            res["action"] = action;
            
            // ClientType::RIDER 대신 상속받은 m_clientType 사용
            session->sendPacket(static_cast<uint8_t>(m_clientType), CmdRider::REQ_WORK_STATUS, res.dump());
        } else {
            sendError(session, CmdRider::REQ_WORK_STATUS, Status::SERVER_ERROR, "DB 업데이트 실패");
        }

    } catch (const std::exception& e) {
        std::cerr << "[handleWorkStatus] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_WORK_STATUS, Status::SERVER_ERROR, "서버 오류");
    }
}

void RiderHandler::handleUpdateGps(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        double lat = req.value("latitude", 0.0);
        double lng = req.value("longitude", 0.0);
        int riderId = getUserIdByFd(session->getFd());

        if (riderId <= 0 || (lat == 0.0 && lng == 0.0)) return;
        auto& db = MariaDBManager::getInstance();

        std::ostringstream q;
        q << "UPDATE users SET latitude = " << lat << ", longitude = " << lng << " WHERE user_id = " << riderId;
        db.executeUpdate(q.str());
        db.executeUpdate("UPDATE rider_profiles SET last_location_at = NOW() WHERE user_id = " + std::to_string(riderId));
    } catch (const std::exception& e) {
        std::cerr << "[handleUpdateGps] 예외: " << e.what() << std::endl;
    }
}