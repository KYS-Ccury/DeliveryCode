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

// ★ 수정: static sendError 함수 삭제 완료 (BaseHandler의 멤버 함수 사용)

void RiderHandler::handleWorkStatus(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "");
        
        // 부모(BaseHandler)의 함수를 사용하여 ID 조회
        int riderId = getUserIdByFd(session->getFd());

        if (riderId <= 0 || action.empty()) { 
            sendError(session, CmdRider::REQ_WORK_STATUS, Status::BAD_REQUEST, "파라미터 오류 또는 비로그인"); 
            return; 
        }

        auto& db = MariaDBManager::getInstance(); 
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

        if (riderId <= 0 || (lat == 0.0 && lng == 0.0)) return; // 오류 시 조용히 무시 (GPS 특성)
        
        auto& db = MariaDBManager::getInstance();

        // ★ 수정: 두 개의 쿼리를 하나로 합쳐서 DB 부하 절반으로 감소 (JOIN UPDATE)
        std::ostringstream q;
        q << "UPDATE users u JOIN rider_profiles rp ON u.user_id = rp.user_id "
          << "SET u.latitude = " << lat << ", u.longitude = " << lng << ", rp.last_location_at = NOW() "
          << "WHERE u.user_id = " << riderId;
          
        db.executeUpdate(q.str());

    } catch (const std::exception& e) {
        std::cerr << "[handleUpdateGps] 예외: " << e.what() << std::endl;
    }
}