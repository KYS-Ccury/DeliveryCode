// AdminHandler.cpp v2 – bemin_db 스키마 기준 완성본
#include "AdminHandler.h"
#include "ChatHandler.h"
#include "RiderHandler.h"
#include "CustomerHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

static void sendErr(Session* s, uint16_t p, uint16_t c, const std::string& m) {
    json r; r["status"] = c; r["message"] = m;
    s->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), p, r.dump());
}
static std::string esc(const std::string& s) {
    std::string o; for (char c : s) { if(c=='\''||c=='\\'||c=='"') o+='\\'; o+=c; } return o;
}

void AdminHandler::registerSession  (int fd, int adminId) { ChatHandler::registerAdmin(fd, adminId); }
void AdminHandler::unregisterSession(int fd)              { ChatHandler::unregisterAdmin(fd); }

void AdminHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    switch (protocol) {
        case CmdCommon::REQ_LOGIN:           handleAdminLogin  (session, body); break;
        case CmdCommon::REQ_LOGOUT:
            ChatHandler::unregisterAdmin(session->getFd());
            { json r; r["status"]=Status::SUCCESS;
              session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdCommon::REQ_LOGOUT, r.dump()); }
            break;
        case CmdAdmin::REQ_MONITOR_ORDERS:   handleGetStats    (session, body); break;
        case CmdAdmin::REQ_RIDER_STATUS:     handleRiderStatus (session, body); break;
        case CmdAdmin::REQ_FORCE_DISPATCH:   handleForceDispatch(session, body); break;
        case CmdAdmin::REQ_FORCE_CANCEL:     handleForceCancel (session, body); break;
        case CmdAdmin::REQ_MANAGE_REVIEW:    handleManageReview(session, body); break;
        case CmdAdmin::REQ_SETTLEMENT_LIST:  handleSettlement  (session, body); break;
        case CmdChat::REQ_CREATE_ROOM:
        case CmdChat::REQ_SEND_MSG:
        case CmdChat::REQ_GET_MSGS:
            ChatHandler::process(session, protocol, body, ClientType::ADMIN);
            break;
        default: sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol"); break;
    }
}

// 101: 관리자 로그인
void AdminHandler::handleAdminLogin(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        std::string id = esc(req.value("id","login_id" ));
        std::string pw = esc(req.value("pw","password" ));
        // 기존 RiderHandlerImpl 방식과 맞춤 (login_id / password 필드)
        if (req.contains("login_id")) id = esc(req["login_id"].get<std::string>());
        if (req.contains("password")) pw = esc(req["password"].get<std::string>());

        auto rows = db.executeQuery(
            "SELECT user_id, name FROM users "
            "WHERE login_id='" + id + "' AND password='" + pw
            + "' AND role='ADMIN' AND status='ACTIVE' LIMIT 1");

        if (rows.empty()) { sendErr(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "로그인 실패"); return; }

        int adminId = std::stoi(rows[0]["user_id"]);
        session->setUserID(adminId);
        session->setUserType(static_cast<uint8_t>(ClientType::ADMIN));
        ChatHandler::registerAdmin(session->getFd(), adminId);

        json res; res["status"] = Status::SUCCESS;
        res["admin_id"] = adminId;
        res["name"]     = rows[0]["name"];
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdCommon::REQ_LOGIN, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, e.what()); }
}

// 510: 주문 모니터링 대시보드
void AdminHandler::handleGetStats(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();

        // 오늘 주문 수
        auto today = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM orders WHERE DATE(created_at)=CURDATE()");
        // 활성 라이더 수
        auto riders = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM rider_profiles WHERE is_online=1");
        // 대기 주문 (PENDING)
        auto pending = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM orders WHERE status='PENDING'");
        // 배달 중 (DELIVERING)
        auto delivering = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM orders WHERE status='DELIVERING'");

        json res; res["status"] = Status::SUCCESS;
        res["today_orders"]  = today.empty()      || today[0]["cnt"].empty()      ? 0 : std::stoi(today[0]["cnt"]);
        res["active_riders"] = riders.empty()     || riders[0]["cnt"].empty()     ? 0 : std::stoi(riders[0]["cnt"]);
        res["pending_orders"]= pending.empty()    || pending[0]["cnt"].empty()    ? 0 : std::stoi(pending[0]["cnt"]);
        res["delivering"]    = delivering.empty() || delivering[0]["cnt"].empty() ? 0 : std::stoi(delivering[0]["cnt"]);
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_MONITOR_ORDERS, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdAdmin::REQ_MONITOR_ORDERS, Status::SERVER_ERROR, e.what()); }
}

// 511: 라이더 현황
void AdminHandler::handleRiderStatus(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, u.phone, rp.vehicle_type, "
            "       rp.is_online, rp.is_working, rp.is_accepting, rp.is_admin_blocked "
            "FROM users u JOIN rider_profiles rp ON rp.user_id = u.user_id "
            "WHERE u.role='RIDER' ORDER BY rp.is_online DESC, u.name");

        json riders = json::array();
        for (auto& r : rows) {
            json rd;
            rd["id"]           = std::stoi(r["user_id"]);
            rd["name"]         = r["name"];
            rd["phone"]        = r.count("phone")        ? r["phone"]       : "";
            rd["vehicle"]      = r.count("vehicle_type") ? r["vehicle_type"]: "";
            rd["is_online"]    = (r.count("is_online")   && r["is_online"]   == "1");
            rd["is_working"]   = (r.count("is_working")  && r["is_working"]  == "1");
            rd["is_accepting"] = (r.count("is_accepting")&& r["is_accepting"]== "1");
            rd["is_blocked"]   = (r.count("is_admin_blocked") && r["is_admin_blocked"] == "1");
            riders.push_back(rd);
        }
        json res; res["status"] = Status::SUCCESS; res["riders"] = riders;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_RIDER_STATUS, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdAdmin::REQ_RIDER_STATUS, Status::SERVER_ERROR, e.what()); }
}

// 512: 강제 배차 (라이더 지정)
void AdminHandler::handleForceDispatch(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int orderId = req.value("order_id", 0);
        int riderId = req.value("rider_id", 0);

        db.executeUpdate(
            "UPDATE orders SET rider_id=" + std::to_string(riderId)
            + ",status='DELIVERING' WHERE order_id=" + std::to_string(orderId));
        db.executeUpdate(
            "INSERT INTO dispatch_logs (order_id,rider_id,result) VALUES ("
            + std::to_string(orderId) + "," + std::to_string(riderId) + ",'ACCEPT')");

        // 라이더에게 Push (NTF_NEW_DISPATCH = 408)
        int riderFd = RiderHandler::getRiderFdById(riderId);
        if (riderFd != -1) {
            auto rows = db.executeQuery(
                "SELECT r.restaurant_name, r.address AS pickup, o.delivery_address AS dest, "
                "       r.base_delivery_fee AS fee "
                "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
                "WHERE o.order_id=" + std::to_string(orderId));
            if (!rows.empty())
                RiderHandler::pushDispatch(riderFd, orderId,
                    rows[0]["restaurant_name"], rows[0]["pickup"],
                    rows[0]["dest"], std::stoi(rows[0]["fee"]));
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_FORCE_DISPATCH, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::SERVER_ERROR, e.what()); }
}

// 513: 강제 취소
void AdminHandler::handleForceCancel(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int orderId = req.value("order_id", 0);
        std::string reason = esc(req.value("reason", "관리자 강제 취소"));

        db.executeUpdate(
            "UPDATE orders SET status='CANCELED',cancel_reason='" + reason
            + "',canceled_at=NOW() WHERE order_id=" + std::to_string(orderId));

        // 고객 Push
        auto rows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id=" + std::to_string(orderId));
        if (!rows.empty() && EpollServer::s_instance) {
            Session* cs = EpollServer::s_instance->getSessionByUserID(std::stoi(rows[0]["customer_id"]));
            if (cs) CustomerHandler::pushOrderStatus(cs, orderId, "CANCELED", "주문이 취소되었습니다: " + reason);
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_FORCE_CANCEL, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdAdmin::REQ_FORCE_CANCEL, Status::SERVER_ERROR, e.what()); }
}

// 520: 리뷰 관리 (블라인드 처리)
void AdminHandler::handleManageReview(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int  reviewId = req.value("review_id", 0);
        bool blind    = req.value("blind",     false);

        // reviews 테이블에 blind 컬럼이 없으면 content를 null로 처리
        if (blind)
            db.executeUpdate("UPDATE reviews SET content=NULL WHERE review_id=" + std::to_string(reviewId));
        else
            db.executeUpdate("UPDATE reviews SET content='" + esc(req.value("content","")) + "' WHERE review_id=" + std::to_string(reviewId));

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_MANAGE_REVIEW, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::SERVER_ERROR, e.what()); }
}

// 500: 정산 목록 조회
void AdminHandler::handleSettlement(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        auto rows = db.executeQuery(
            "SELECT s.settlement_id, r.restaurant_name, s.period_start, s.period_end, "
            "       s.net_amount, s.status "
            "FROM settlements s JOIN restaurants r ON r.restaurant_id = s.restaurant_id "
            "ORDER BY s.period_end DESC LIMIT 100");

        json list = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]      = std::stoi(r["settlement_id"]);
            s["store"]   = r["restaurant_name"];
            s["from"]    = r["period_start"];
            s["to"]      = r["period_end"];
            s["amount"]  = r.count("net_amount") && !r["net_amount"].empty() ? std::stoi(r["net_amount"]) : 0;
            s["status"]  = r["status"];
            list.push_back(s);
        }
        json res; res["status"] = Status::SUCCESS; res["settlements"] = list;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_SETTLEMENT_LIST, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdAdmin::REQ_SETTLEMENT_LIST, Status::SERVER_ERROR, e.what()); }
}
