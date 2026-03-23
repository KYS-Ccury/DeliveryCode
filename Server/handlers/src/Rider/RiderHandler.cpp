// ============================================================
//  RiderHandler.cpp  [호출부 - Dispatcher 진입점]
//  실제 기능 구현은 RiderHandlerImpl.cpp 에 있습니다.
// ============================================================
#include "RiderHandler.h"
#include "EpollServer.h"
#include "Session.h"
#include "Packet.h"
#include "MariaDBManager.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mutex>

using json = nlohmann::json;

// ─── 정적 멤버 초기화 ─────────────────────────────────────
std::unordered_map<int, int> RiderHandler::s_fdToRider;
std::unordered_map<int, int> RiderHandler::s_riderToFd;
std::mutex                   RiderHandler::s_sessionMtx;

// ─── 세션 등록 / 해제 / 조회 ──────────────────────────────
void RiderHandler::registerSession(int fd, int riderId) {
    std::lock_guard<std::mutex> lk(s_sessionMtx);
    s_fdToRider[fd]      = riderId;
    s_riderToFd[riderId] = fd;
}

void RiderHandler::unregisterSession(int fd) {
    std::lock_guard<std::mutex> lk(s_sessionMtx);
    auto it = s_fdToRider.find(fd);
    if (it != s_fdToRider.end()) {
        s_riderToFd.erase(it->second);
        s_fdToRider.erase(it);
    }
}

int RiderHandler::getRiderIdByFd(int fd) {
    std::lock_guard<std::mutex> lk(s_sessionMtx);
    auto it = s_fdToRider.find(fd);
    return (it != s_fdToRider.end()) ? it->second : 0;
}

// ─── 메인 디스패치 ────────────────────────────────────────
void RiderHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    switch (protocol) {
        // 공통 인증
        case CmdCommon::REQ_SIGNUP:      handleSignup    (session, jsonBody); break;
        case CmdCommon::REQ_LOGIN:       handleLogin     (session, jsonBody); break;
        case CmdCommon::REQ_LOGOUT:      handleLogout    (session, jsonBody); break;
        case CmdCommon::REQ_GET_PROFILE: handleGetProfile(session, jsonBody); break;

        // 라이더 전용
        case CmdRider::REQ_DISPATCH_LIST:   handleDispatchList  (session, jsonBody); break;
        case CmdRider::REQ_ACCEPT_DISPATCH: handleAcceptDispatch(session, jsonBody); break;
        case CmdRider::REQ_REJECT_DISPATCH: handleRejectDispatch(session, jsonBody); break;
        case CmdRider::REQ_PICKUP_DONE:     handlePickupDone    (session, jsonBody); break;
        case CmdRider::REQ_DELIVERY_DONE:   handleDeliveryDone  (session, jsonBody); break;
        case CmdRider::REQ_MY_DISPATCHES:   handleMyDispatches  (session, jsonBody); break;
        case CmdRider::REQ_WORK_STATUS:     handleWorkStatus    (session, jsonBody); break;
        case CmdRider::REQ_SEND_GPS:        handleUpdateGps     (session, jsonBody); break;

        default:
            std::cerr << "[Rider] 알 수 없는 프로토콜: " << protocol << std::endl;
            break;
    }
}

// ─── Push: 신규 배차 알림 (408) ───────────────────────────
bool RiderHandler::pushDispatch(int riderFd, int orderId,
                                const std::string& storeName,
                                const std::string& pickupAddr,
                                const std::string& destAddr,
                                int deliveryFee)
{
    if (!EpollServer::s_instance) {
        std::cerr << "[pushDispatch] EpollServer 인스턴스 없음" << std::endl;
        return false;
    }

    auto session = EpollServer::s_instance->getSession(riderFd);
    if (!session) {
        std::cerr << "[pushDispatch] riderFd=" << riderFd << " 세션 없음" << std::endl;
        return false;
    }

    json push;
    push["order_id"]     = orderId;
    push["store_name"]   = storeName;
    push["pickup_addr"]  = pickupAddr;
    push["dest_addr"]    = destAddr;
    push["delivery_fee"] = deliveryFee;

    bool ok = session->sendPacket(
        static_cast<uint8_t>(ClientType::RIDER),
        CmdRider::NTF_NEW_DISPATCH,
        push.dump()
    );

    std::cout << "[Rider Push] orderId=" << orderId
              << " riderFd=" << riderFd
              << (ok ? " → 전송 성공" : " → 전송 실패") << std::endl;
    return ok;
}
