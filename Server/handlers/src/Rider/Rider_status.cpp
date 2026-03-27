#include "RiderHandler.h"
#include "RiderDB.h"
#include "Protocol.h"
#include "Session.h"
#include <iostream>

using json = nlohmann::json;

void RiderHandler::handleWorkStatus(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string action      = req.value("action", "");
        std::string vehicleType = req.value("vehicle_type", "BIKE");

        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0 || action.empty()) {
            sendError(session, CmdRider::REQ_WORK_STATUS,
                      Status::BAD_REQUEST, "파라미터 오류 또는 비로그인");
            return;
        }

        bool ok = RiderDB::getInstance().setWorkStatus(riderId, action, vehicleType);

        if (!ok) {
            sendError(session, CmdRider::REQ_WORK_STATUS,
                      Status::BAD_REQUEST, "알 수 없는 action 또는 DB 오류: " + action);
            return;
        }

        json res;
        res["status"] = Status::SUCCESS;
        res["action"] = action;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdRider::REQ_WORK_STATUS, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleWorkStatus] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_WORK_STATUS,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

void RiderHandler::handleUpdateGps(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        double lat = req.value("latitude",  0.0);
        double lng = req.value("longitude", 0.0);
        int riderId = getUserIdByFd(session->getFd());

        if (riderId <= 0 || (lat == 0.0 && lng == 0.0)) return;

        RiderDB::getInstance().updateGps(riderId, lat, lng);

    } catch (const std::exception& e) {
        std::cerr << "[handleUpdateGps] 예외: " << e.what() << std::endl;
    }
}
