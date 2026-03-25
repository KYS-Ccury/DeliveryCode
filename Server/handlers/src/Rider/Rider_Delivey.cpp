#include "RiderHandler.h"
#include "Session.h"
#include "Struct.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "EpollServer.h"

using json = nlohmann::json;

// ================================================================
//  handleDispatchList  (REQ_DISPATCH_LIST = 400)
//  배차 대기 주문 목록 (status='ACCEPTED', rider_id IS NULL)
// ================================================================
void RiderHandler::handleDispatchList(Session* session, const std::string&) {
    try {
        int riderId = getUserIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdRider::REQ_DISPATCH_LIST,
                      Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       r.address AS pickup_addr, o.delivery_address AS dest_addr, "
            "       r.base_delivery_fee AS delivery_fee, o.total_price, "
            "       TIMESTAMPDIFF(SECOND, o.created_at, NOW()) AS elapsed_sec "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.status='ACCEPTED' AND o.rider_id IS NULL "
            "  AND o.delivery_method='DELIVERY' "
            "ORDER BY o.created_at ASC");

        json res;
        res["status"] = Status::SUCCESS;
        res["orders"] = json::array();
        for (const auto& row : rows) {
            json item;
            item["order_id"]     = std::stoi(row.at("order_id"));
            item["store_name"]   = row.at("store_name");
            item["pickup_addr"]  = row.at("pickup_addr");
            item["dest_addr"]    = row.at("dest_addr");
            item["delivery_fee"] = std::stoi(row.at("delivery_fee"));
            item["total_price"]  = std::stoi(row.at("total_price"));
            item["elapsed_sec"]  = row.at("elapsed_sec").empty() ? 0
                                   : std::stoi(row.at("elapsed_sec"));
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
//  배차 수락: ACCEPTED → DELIVERING
//  executeTransaction 사용 → 데드락 없음
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

        auto& db = MariaDBManager::getInstance();

        // 트랜잭션으로 동시 수락 방지
        bool txOk = db.executeTransaction([&]() -> bool {
            // 아직 배차 안 된 ACCEPTED 주문인지 확인
            auto check = db.executeQuery(
                "SELECT order_id FROM orders "
                "WHERE order_id=" + std::to_string(orderId) +
                " AND status='ACCEPTED' AND rider_id IS NULL LIMIT 1");
            if (check.empty()) return false;

            // 라이더 배정 + DELIVERING
            bool ok = db.executeUpdate(
                "UPDATE orders SET rider_id=" + std::to_string(riderId) +
                ", status='DELIVERING' "
                "WHERE order_id=" + std::to_string(orderId));
            if (!ok) return false;

            db.executeUpdate(
                "INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES ("
                + std::to_string(orderId) + "," + std::to_string(riderId) + ",'ACCEPT')");
            db.executeUpdate(
                "INSERT INTO order_status_logs "
                "(order_id, from_status, to_status, changed_by) VALUES ("
                + std::to_string(orderId) + ",'ACCEPTED','DELIVERING',"
                + std::to_string(riderId) + ")");
            return true;
        });

        if (!txOk) {
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH,
                      Status::NOT_FOUND,
                      "이미 다른 라이더가 수락했거나 유효하지 않은 주문입니다.");
            return;
        }

        // 주문 상세 응답
        auto detail = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       r.address AS pickup_addr, o.delivery_address AS dest_addr, "
            "       r.phone AS store_phone, r.base_delivery_fee AS delivery_fee, "
            "       o.total_price "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderId));

        json res;
        res["status"]   = Status::SUCCESS;
        res["order_id"] = orderId;
        if (!detail.empty()) {
            const auto& d = detail[0];
            std::ostringstream code;
            code << "ORD" << std::setw(6) << std::setfill('0') << orderId;
            res["order_code"]  = code.str();
            res["store_name"]  = d.at("store_name");
            res["pickup_addr"] = d.at("pickup_addr");
            res["store_phone"] = d.count("store_phone") ? d.at("store_phone") : "";
            res["dest_addr"]   = d.at("dest_addr");
            res["delivery_fee"]= std::stoi(d.at("delivery_fee"));
            res["total_price"] = std::stoi(d.at("total_price"));
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_ACCEPT_DISPATCH, res.dump());

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

        auto& db = MariaDBManager::getInstance();
        db.executeUpdate(
            "INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES ("
            + std::to_string(orderId) + "," + std::to_string(riderId)
            + ",'" + escapeStr(reason) + "')");

        json res; res["status"] = Status::SUCCESS;
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
//  픽업 완료 확인 응답 (상태는 DELIVERING 유지, 로그 기록)
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

        auto& db = MariaDBManager::getInstance();
        db.executeUpdate(
            "INSERT INTO order_status_logs "
            "(order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderId) +
            ",'DELIVERING','DELIVERING'," + std::to_string(riderId) + ")");

        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "픽업 완료, 배달을 시작합니다.";
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_PICKUP_DONE, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handlePickupDone] " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_PICKUP_DONE,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleDeliveryDone  (REQ_DELIVERY_DONE = 404)
//  배달 완료: DELIVERING → DONE + 수익 기록 + 고객 Push
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

        auto& db = MariaDBManager::getInstance();
        int deliveryFee = 0;
        int customerId  = 0;

        bool txOk = db.executeTransaction([&]() -> bool {
            auto check = db.executeQuery(
                "SELECT o.order_id, r.base_delivery_fee AS fee, o.customer_id "
                "FROM orders o "
                "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
                "WHERE o.order_id=" + std::to_string(orderId) +
                " AND o.rider_id=" + std::to_string(riderId) +
                " AND o.status='DELIVERING' LIMIT 1");

            if (check.empty()) return false;

            deliveryFee = std::stoi(check[0].at("fee"));
            customerId  = std::stoi(check[0].at("customer_id"));

            db.executeUpdate(
                "UPDATE orders SET status='DONE', is_masked=TRUE "
                "WHERE order_id=" + std::to_string(orderId));
            db.executeUpdate(
                "INSERT INTO rider_earnings (rider_id, order_id, delivery_fee) VALUES ("
                + std::to_string(riderId) + ","
                + std::to_string(orderId) + ","
                + std::to_string(deliveryFee) + ")");
            db.executeUpdate(
                "INSERT INTO order_status_logs "
                "(order_id, from_status, to_status, changed_by) VALUES ("
                + std::to_string(orderId) +
                ",'DELIVERING','DONE'," + std::to_string(riderId) + ")");
            return true;
        });

        if (!txOk) {
            sendError(session, CmdRider::REQ_DELIVERY_DONE, Status::NOT_FOUND,
                      "유효하지 않은 주문이거나 이미 완료된 주문입니다.");
            return;
        }

        // 고객에게 배달 완료 Push
        if (EpollServer::s_instance && customerId > 0) {
            auto* custSession = EpollServer::s_instance->getSessionByUserID(customerId);
            if (custSession) {
                json ntf;
                ntf["order_id"] = orderId;
                ntf["status"]   = 3;
                ntf["message"]  = "배달이 완료되었습니다.";
                custSession->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                                        CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
            }
        }

        json res;
        res["status"]       = Status::SUCCESS;
        res["delivery_fee"] = deliveryFee;
        res["message"]      = "배달이 완료되었습니다. 수고하셨습니다!";
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_DELIVERY_DONE, res.dump());

        std::cout << "[Rider] 배달 완료: orderId=" << orderId
                  << " fee=" << deliveryFee << std::endl;

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
        auto& db = MariaDBManager::getInstance();

        if (summaryOnly) {
            auto rows = db.executeQuery(
                "SELECT COUNT(*) AS cnt, "
                "       IFNULL(SUM(re.delivery_fee), 0) AS total "
                "FROM orders o "
                "JOIN rider_earnings re ON re.order_id = o.order_id "
                "WHERE o.rider_id=" + std::to_string(riderId) +
                "  AND DATE(o.created_at)=CURDATE() AND o.status='DONE'");

            json res;
            res["status"]      = Status::SUCCESS;
            res["today_count"] = rows.empty() ? 0 : std::stoi(rows[0].at("cnt"));
            res["today_fee"]   = rows.empty() ? 0 : std::stoi(rows[0].at("total"));
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdRider::REQ_MY_DISPATCHES, res.dump());
            return;
        }

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, o.status, "
            "       IFNULL(re.delivery_fee,0) AS delivery_fee, "
            "       DATE_FORMAT(o.created_at,'%m/%d %H:%i') AS created_at "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "LEFT JOIN rider_earnings re "
            "  ON re.order_id=o.order_id AND re.rider_id=" + std::to_string(riderId) +
            " WHERE o.rider_id=" + std::to_string(riderId) +
            "   AND o.status='DONE' "
            "ORDER BY o.created_at DESC LIMIT 50");

        json res;
        res["status"]  = Status::SUCCESS;
        res["records"] = json::array();
        for (const auto& row : rows) {
            int oid = std::stoi(row.at("order_id"));
            std::ostringstream code;
            code << "ORD" << std::setw(6) << std::setfill('0') << oid;
            json item;
            item["order_id"]    = oid;
            item["order_code"]  = code.str();
            item["store_name"]  = row.at("store_name");
            item["delivery_fee"]= row.at("delivery_fee").empty() ? 0
                                  : std::stoi(row.at("delivery_fee"));
            item["status"]      = row.at("status");
            item["created_at"]  = row.at("created_at");
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
//  pushDispatch
// ================================================================
bool RiderHandler::pushDispatch(int riderFd, int orderId,
                                const std::string& storeName,
                                const std::string& pickupAddr,
                                const std::string& destAddr,
                                int deliveryFee)
{
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
