#include "RiderHandler.h"
#include "RiderDB.h"
#include "EpollServer.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// ── Push 헬퍼: 고객에게 주문 상태 알림 전송 ──────────────────
// statusCode: 1=배차완료 2=픽업완료(배달중) 3=배달완료
static void pushToCustomer(int customerId, int orderId,
                            int statusCode, const std::string& message) {
    if (!EpollServer::s_instance || customerId <= 0) return;
    auto sess = EpollServer::s_instance->getSessionByUserID(customerId);
    if (!sess) return;
    json ntf;
    ntf["order_id"] = orderId;
    ntf["status"]   = statusCode;
    ntf["message"]  = message;
    sess->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                     CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
}

// ── Push 헬퍼: 사장님에게 배달 상태 알림 전송 ───────────────
// statusCode: 1=라이더픽업완료 2=배달완료
static void pushToOwner(int ownerId, int orderId,
                         int statusCode, const std::string& message) {
    if (!EpollServer::s_instance || ownerId <= 0) return;
    auto sess = EpollServer::s_instance->getSessionByUserID(ownerId);
    if (!sess) return;
    json ntf;
    ntf["order_id"] = orderId;
    ntf["status"]   = statusCode;
    ntf["message"]  = message;
    sess->sendPacket(static_cast<uint8_t>(ClientType::OWNER),
                     CmdOwner::NTF_RIDER_MATCHED, ntf.dump());
}

// ================================================================
//  handleDispatchList  (REQ_DISPATCH_LIST = 400)
// ================================================================
void RiderHandler::handleDispatchList(Session* session, const std::string&) {
    try {
        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdRider::REQ_DISPATCH_LIST,
                      Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto orders = RiderDB::getInstance().queryDispatchList();

        json res;
        res["status"] = Status::SUCCESS;
        res["orders"] = json::array();
        for (const auto& o : orders) {
            json item;
            item["order_id"]     = o.orderId;
            item["store_name"]   = o.storeName;
            item["pickup_addr"]  = o.pickupAddr;
            item["dest_addr"]    = o.destAddr;
            item["delivery_fee"] = o.deliveryFee;
            item["total_price"]  = o.totalPrice;
            item["elapsed_sec"]  = o.elapsedSec;
            res["orders"].push_back(item);
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_DISPATCH_LIST, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleDispatchList] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_DISPATCH_LIST,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleAcceptDispatch  (REQ_ACCEPT_DISPATCH = 401)
//  ACCEPTED → DELIVERING
//  Push: 고객(배차완료), 사장님(라이더 배정)
// ================================================================
void RiderHandler::handleAcceptDispatch(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = getUserIdByFd(session->getFd());

        if (orderId <= 0) {
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH,
                      Status::BAD_REQUEST, "order_id 누락");
            return;
        }
        if (riderId <= 0) {
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH,
                      Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto result = RiderDB::getInstance().acceptDispatch(orderId, riderId);

        if (!result.ok) {
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH, Status::NOT_FOUND,
                      "이미 다른 라이더가 수락했거나 유효하지 않은 주문입니다.");
            return;
        }

        // 라이더에게 응답
        json res;
        res["status"]   = Status::SUCCESS;
        res["order_id"] = orderId;
        if (result.detail.found) {
            const auto& d = result.detail;
            res["order_code"]   = "ORD" + std::to_string(orderId);
            res["store_name"]   = d.storeName;
            res["pickup_addr"]  = d.pickupAddr;
            res["store_phone"]  = d.storePhone;
            res["dest_addr"]    = d.destAddr;
            res["delivery_fee"] = d.deliveryFee;
            res["total_price"]  = d.totalPrice;
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_ACCEPT_DISPATCH, res.dump());

        // ── 실시간 상태 Push ─────────────────────────────────
        // 고객: 라이더가 배정되어 픽업을 향해 출발 중
        pushToCustomer(result.detail.customerId, orderId, 1,
                       "라이더가 배정되었습니다. 픽업을 위해 출발합니다.");
        // 사장님: 라이더 배정 완료, 픽업 대기
        pushToOwner(result.detail.ownerId, orderId, 1,
                    "라이더가 배정되었습니다. 픽업을 기다려 주세요.");

        std::cout << "[Rider] 배차 수락: orderId=" << orderId
                  << " riderId=" << riderId << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleAcceptDispatch] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_ACCEPT_DISPATCH,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleRejectDispatch  (REQ_REJECT_DISPATCH = 402)
// ================================================================
void RiderHandler::handleRejectDispatch(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        std::string reason = req.value("reason", "MANUAL");
        int riderId = getUserIdByFd(session->getFd());

        if (riderId <= 0 || orderId <= 0) {
            sendError(session, CmdRider::REQ_REJECT_DISPATCH,
                      Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        RiderDB::getInstance().rejectDispatch(orderId, riderId, reason);

        json res;
        res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_REJECT_DISPATCH, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleRejectDispatch] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_REJECT_DISPATCH,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handlePickupDone  (REQ_PICKUP_DONE = 403)
//  DELIVERING → WAITING_PICKUP
//  Push: 고객(픽업완료/배달중), 사장님(픽업완료)
// ================================================================
void RiderHandler::handlePickupDone(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = getUserIdByFd(session->getFd());

        if (orderId <= 0 || riderId <= 0) {
            sendError(session, CmdRider::REQ_PICKUP_DONE,
                      Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        auto result = RiderDB::getInstance().pickupDone(orderId, riderId);

        if (!result.ok) {
            sendError(session, CmdRider::REQ_PICKUP_DONE, Status::NOT_FOUND,
                      "유효하지 않은 주문이거나 이미 처리된 주문입니다.");
            return;
        }

        // 라이더에게 응답
        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "픽업 완료, 배달을 시작합니다.";
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_PICKUP_DONE, res.dump());

        // ── 실시간 상태 Push ─────────────────────────────────
        // 고객: 음식을 픽업해서 배달 중
        pushToCustomer(result.customerId, orderId, 2,
                       "라이더가 음식을 픽업했습니다. 배달 중입니다.");
        // 사장님: 픽업 완료
        pushToOwner(result.ownerId, orderId, 1,
                    "라이더가 음식을 픽업했습니다.");

        std::cout << "[Rider] 픽업 완료: orderId=" << orderId
                  << " riderId=" << riderId << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handlePickupDone] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_PICKUP_DONE,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleDeliveryDone  (REQ_DELIVERY_DONE = 404)
//  WAITING_PICKUP → DONE
//  Push: 고객(배달완료), 사장님(배달완료)
// ================================================================
void RiderHandler::handleDeliveryDone(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = getUserIdByFd(session->getFd());

        if (orderId <= 0 || riderId <= 0) {
            sendError(session, CmdRider::REQ_DELIVERY_DONE,
                      Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        auto result = RiderDB::getInstance().deliveryDone(orderId, riderId);

        if (!result.ok) {
            sendError(session, CmdRider::REQ_DELIVERY_DONE, Status::NOT_FOUND,
                      "유효하지 않은 주문이거나 이미 완료된 주문입니다.");
            return;
        }

        // 라이더에게 응답
        json res;
        res["status"]       = Status::SUCCESS;
        res["delivery_fee"] = result.deliveryFee;
        res["message"]      = "배달이 완료되었습니다. 수고하셨습니다!";
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_DELIVERY_DONE, res.dump());

        // ── 실시간 상태 Push ─────────────────────────────────
        // 고객: 배달 완료
        pushToCustomer(result.customerId, orderId, 3,
                       "배달이 완료되었습니다. 맛있게 드세요!");
        // 사장님: 배달 완료
        pushToOwner(result.ownerId, orderId, 2,
                    "배달이 완료되었습니다.");

        std::cout << "[Rider] 배달 완료: orderId=" << orderId
                  << " fee=" << result.deliveryFee << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleDeliveryDone] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_DELIVERY_DONE,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleMyDispatches  (REQ_MY_DISPATCHES = 405)
// ================================================================
void RiderHandler::handleMyDispatches(Session* session, const std::string& jsonBody) {
    try {
        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdRider::REQ_MY_DISPATCHES,
                      Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        json req;
        try { req = jsonBody.empty() ? json::object() : json::parse(jsonBody); }
        catch (...) { req = json::object(); }

        bool summaryOnly = req.value("summary_only", false);
        auto& rdb = RiderDB::getInstance();

        if (summaryOnly) {
            auto summary = rdb.queryTodaySummary(riderId);
            json res;
            res["status"]      = Status::SUCCESS;
            res["today_count"] = summary.todayCount;
            res["today_fee"]   = summary.todayFee;
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdRider::REQ_MY_DISPATCHES, res.dump());
            return;
        }

        auto records = rdb.queryMyDispatches(riderId);
        json res;
        res["status"]  = Status::SUCCESS;
        res["records"] = json::array();
        for (const auto& rec : records) {
            json item;
            item["order_id"]     = rec.orderId;
            item["order_code"]   = rec.orderCode;
            item["store_name"]   = rec.storeName;
            item["delivery_fee"] = rec.deliveryFee;
            item["status"]       = rec.status;
            item["created_at"]   = rec.createdAt;
            res["records"].push_back(item);
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_MY_DISPATCHES, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleMyDispatches] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_MY_DISPATCHES,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  pushDispatch  (Admin에서 외부 호출)
// ================================================================
bool RiderHandler::pushDispatch(int riderFd, int orderId,
                                const std::string& storeName,
                                const std::string& pickupAddr,
                                const std::string& destAddr,
                                int deliveryFee) {
    if (!EpollServer::s_instance) return false;
    auto session = EpollServer::s_instance->getSession(riderFd);
    if (!session) return false;

    json push;
    push["order_id"]     = orderId;
    push["store_name"]   = storeName;
    push["pickup_addr"]  = pickupAddr;
    push["dest_addr"]    = destAddr;
    push["delivery_fee"] = deliveryFee;

    bool ok = session->sendPacket(static_cast<uint8_t>(m_clientType),
                                  CmdRider::NTF_NEW_DISPATCH, push.dump());
    std::cout << "[Rider Push] orderId=" << orderId
              << " fd=" << riderFd
              << (ok ? " 성공" : " 실패") << std::endl;
    return ok;
}
