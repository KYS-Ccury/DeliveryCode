#include "RiderHandler.h"
#include "Session.h"
#include "Packet.h"          // CmdRider, ClientType, Status 포함
#include "MariaDBManager.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include "Types.h"

using json = nlohmann::json;

void RiderHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    // 너무 잦은 로그(GPS 등)는 콘솔 부하를 주므로, 필요에 따라 분기 처리하거나 주석 처리하는 것이 좋습니다.
    // std::cout << "[RiderHandler] 라이더 요청 수신 - 프로토콜: " << protocol << std::endl;

    switch (protocol) {
        case CmdRider::REQ_SEND_GPS:      // 위치 전송 (407)
            handleUpdateGps(session, jsonBody);
            break;

        case CmdRider::REQ_PICKUP_DONE:   // 픽업 완료 (403)
            handlePickupDone(session, jsonBody);
            break;

        default:
            std::cerr << "[Rider] 알 수 없는 라이더 프로토콜: " << protocol << std::endl;
            break;
    }
}

// ---------------------------------------------------------
// [기능 구현] 1. 라이더 GPS 위치 업데이트 (기능 개발 중)
// ---------------------------------------------------------
void RiderHandler::handleUpdateGps(Session* session, const std::string& jsonBody) {
    try {
        json req = jsonBody.empty() ? json{} : json::parse(jsonBody);

        // 초당 수없이 들어오는 GPS 데이터 (가장 가벼워야 함)
        std::string riderId = req.value("rider_id", "");
        double lat = req.value("latitude", 0.0);
        double lng = req.value("longitude", 0.0);

        // DB Update 또는 메모리 캐시(Redis 등)에 위치 업데이트 구현 예정
        // auto& db = MariaDBManager::getInstance();
        // db.executeUpdate("UPDATE riders SET lat=..., lng=... WHERE id=" + riderId);

        // 작성하신 대로 GPS 갱신은 네트워크 비용 절감을 위해 응답 패킷 없이 바로 리턴
        return; 

    } catch (const std::exception& e) {
        std::cerr << "[handleUpdateGps] JSON 예외 발생: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------
// [기능 구현] 2. 픽업 완료 처리 (기능 개발 중)
// ---------------------------------------------------------
void RiderHandler::handlePickupDone(Session* session, const std::string& jsonBody) {
    try {
        json req = jsonBody.empty() ? json{} : json::parse(jsonBody);
        
        std::string orderId = req.value("order_id", "");

        // DB 연동 구현 예정: 픽업 완료 처리 (상태를 'DELIVERING' 등으로 변경)
        // auto& db = MariaDBManager::getInstance();
        // db.executeUpdate("UPDATE orders SET status='DELIVERING' WHERE order_id=" + orderId);

        json res;
        res["status"] = Status::SUCCESS;
        res["message"] = "픽업 확인, 배달을 시작합니다.";

        // 응답 전송 (클라이언트 타입 3: 라이더)
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER), 
                            CmdRider::REQ_PICKUP_DONE, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handlePickupDone] JSON 예외 발생: " << e.what() << std::endl;
    }
}