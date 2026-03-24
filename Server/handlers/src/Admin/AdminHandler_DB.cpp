// AdminHandler_DB.cpp – 실제 DB 스키마 기반 완성
// REQ_MONITOR_ORDERS, REQ_RIDER_STATUS, REQ_FORCE_DISPATCH,
// REQ_FORCE_CANCEL, REQ_SETTLEMENT_LIST, REQ_MANAGE_REVIEW, 로그인
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
    json r; r["status"]=c; r["message"]=m;
    s->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), p, r.dump());
}
static std::string esc(const std::string& v) {
    std::string o; for(char c:v){if(c=='\''||c=='\\'||c=='"')o+='\\';o+=c;} return o;
}

void AdminHandler::registerSession  (int fd, int adminId) { ChatHandler::registerAdmin  (fd, adminId); }
void AdminHandler::unregisterSession(int fd)               { ChatHandler::unregisterAdmin(fd);          }

void AdminHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    switch (protocol) {
        case CmdCommon::REQ_LOGIN:             handleAdminLogin   (session, body); break;
        case CmdCommon::REQ_LOGOUT:
            ChatHandler::unregisterAdmin(session->getFd());
            { json r; r["status"]=Status::SUCCESS;
              session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN),CmdCommon::REQ_LOGOUT,r.dump()); }
            break;
        case CmdAdmin::REQ_MONITOR_ORDERS:     handleGetStats     (session, body); break;
        case CmdAdmin::REQ_RIDER_STATUS:       handleRiderStatus  (session, body); break;
        case CmdAdmin::REQ_FORCE_DISPATCH:     handleForceDispatch(session, body); break;
        case CmdAdmin::REQ_FORCE_CANCEL:       handleForceCancel  (session, body); break;
        case CmdAdmin::REQ_SETTLEMENT_LIST:    handleSettlement   (session, body); break;
        case CmdAdmin::REQ_MANAGE_REVIEW:      handleBanUser      (session, body); break;
        case CmdChat::REQ_CREATE_ROOM:
        case CmdChat::REQ_SEND_MSG:
        case CmdChat::REQ_GET_MSGS:
            ChatHandler::process(session, protocol, body, ClientType::ADMIN); break;
        default:
            sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
    }
}

// ─────────────────────────────────────────────────
// 관리자 로그인
// ─────────────────────────────────────────────────
void AdminHandler::handleAdminLogin(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        std::string id=esc(req.value("id","")), pw=esc(req.value("pw",""));

        auto rows = db.executeQuery(
            "SELECT user_id, name FROM users "
            "WHERE login_id='" + id + "' AND password='" + pw + "' "
            "AND role='ADMIN' AND status='ACTIVE' LIMIT 1");
        if (rows.empty()) { sendErr(session,CmdCommon::REQ_LOGIN,Status::UNAUTHORIZED,"관리자 인증 실패"); return; }

        int uid = std::stoi(rows[0]["user_id"]);
        session->setUserID(uid);
        session->setUserType(static_cast<uint8_t>(ClientType::ADMIN));
        AdminHandler::registerSession(session->getFd(), uid);

        json res; res["status"]=Status::SUCCESS; res["user_id"]=uid; res["name"]=rows[0]["name"];
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdCommon::REQ_LOGIN, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdCommon::REQ_LOGIN,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 510: 대기 주문 모니터링 (실시간 통계)
// ─────────────────────────────────────────────────
void AdminHandler::handleGetStats(Session* session, const std::string&) {
    try {
        auto& db = MariaDBManager::getInstance();

        // 오늘 주문 수
        auto ordRows = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM orders WHERE DATE(created_at)=CURDATE()");
        int todayOrders = ordRows.empty() ? 0 : std::stoi(ordRows[0]["cnt"]);

        // 현재 배달 중 라이더 수
        auto rRows = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM rider_profiles WHERE is_working=1 AND is_online=1");
        int activeRiders = rRows.empty() ? 0 : std::stoi(rRows[0]["cnt"]);

        // 대기 중인 주문 목록 (PENDING)
        auto pendRows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name, u.name AS cust_name, "
            "o.total_price, o.created_at "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id=o.restaurant_id "
            "JOIN users u ON u.user_id=o.customer_id "
            "WHERE o.status='PENDING' ORDER BY o.created_at ASC LIMIT 20");

        json pending = json::array();
        for (auto& r : pendRows) {
            json o; o["order_id"]=std::stoi(r["order_id"]);
            o["store_name"]=r["restaurant_name"]; o["cust_name"]=r["cust_name"];
            o["total_price"]=std::stoi(r.count("total_price")?r["total_price"]:"0");
            o["order_time"]=r.count("created_at")?r["created_at"]:"";
            pending.push_back(o);
        }

        json res; res["status"]=Status::SUCCESS;
        res["today_orders"]=todayOrders; res["active_riders"]=activeRiders;
        res["pending_orders"]=pending;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_MONITOR_ORDERS, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdAdmin::REQ_MONITOR_ORDERS,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 511: 라이더 현황 조회
// ─────────────────────────────────────────────────
void AdminHandler::handleRiderStatus(Session* session, const std::string&) {
    try {
        auto& db = MariaDBManager::getInstance();
        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, u.phone, rp.vehicle_type, "
            "rp.is_working, rp.is_online, rp.is_accepting "
            "FROM users u JOIN rider_profiles rp ON rp.user_id=u.user_id "
            "WHERE u.role='RIDER' AND u.status='ACTIVE'");

        json riders = json::array();
        for (auto& r : rows) {
            json rd;
            rd["user_id"]      = std::stoi(r["user_id"]);
            rd["name"]         = r["name"];
            rd["phone"]        = r.count("phone")        ? r["phone"]        : "";
            rd["vehicle_type"] = r.count("vehicle_type") ? r["vehicle_type"] : "";
            rd["is_working"]   = (r.count("is_working")  && r["is_working"]  =="1");
            rd["is_online"]    = (r.count("is_online")   && r["is_online"]   =="1");
            rd["is_accepting"] = (r.count("is_accepting")&& r["is_accepting"]=="1");
            riders.push_back(rd);
        }
        json res; res["status"]=Status::SUCCESS; res["riders"]=riders;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_RIDER_STATUS, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdAdmin::REQ_RIDER_STATUS,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 512: 강제 배차 (관리자 → 라이더 push)
// 요청: { order_id, rider_id }
// ─────────────────────────────────────────────────
void AdminHandler::handleForceDispatch(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int ordID   = req.value("order_id", 0);
        int riderID = req.value("rider_id",  0);

        // orders.rider_id 배정
        db.executeUpdate(
            "UPDATE orders SET rider_id=" + std::to_string(riderID)
            + ", status='DELIVERING' WHERE order_id=" + std::to_string(ordID));

        // dispatch_logs INSERT
        db.executeUpdate(
            "INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES ("
            + std::to_string(ordID) + "," + std::to_string(riderID) + ",'ACCEPT')");

        // 라이더에게 NTF_NEW_DISPATCH(408) Push
        int riderFd = RiderHandler::getRiderFdById(riderID);
        if (riderFd != -1) {
            auto ordRows = db.executeQuery(
                "SELECT r.restaurant_name, r.address AS pickup_addr, "
                "o.delivery_address, r.base_delivery_fee "
                "FROM orders o JOIN restaurants r ON r.restaurant_id=o.restaurant_id "
                "WHERE o.order_id=" + std::to_string(ordID));
            if (!ordRows.empty()) {
                RiderHandler::pushDispatch(riderFd, ordID,
                    ordRows[0]["restaurant_name"],
                    ordRows[0].count("pickup_addr")   ? ordRows[0]["pickup_addr"]   : "",
                    ordRows[0].count("delivery_address")? ordRows[0]["delivery_address"]:"",
                    std::stoi(ordRows[0].count("base_delivery_fee")?ordRows[0]["base_delivery_fee"]:"0"));
            }
        }

        json res; res["status"]=Status::SUCCESS; res["message"]="강제 배차 완료";
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_FORCE_DISPATCH, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdAdmin::REQ_FORCE_DISPATCH,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 513: 강제 취소
// ─────────────────────────────────────────────────
void AdminHandler::handleForceCancel(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int ordID = req.value("order_id", 0);
        std::string reason = esc(req.value("reason", "관리자 강제 취소"));

        db.executeUpdate(
            "UPDATE orders SET status='CANCELED', cancel_reason='" + reason
            + "', canceled_at=NOW() WHERE order_id=" + std::to_string(ordID));

        // 고객에게 Push
        auto cRows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id="+std::to_string(ordID));
        if (!cRows.empty()) {
            int cid = std::stoi(cRows[0]["customer_id"]);
            if (EpollServer::s_instance) {
                if (auto* cs = EpollServer::s_instance->getSessionByUserID(cid))
                    CustomerHandler::pushOrderStatus(cs, ordID, 0, "관리자에 의해 주문이 취소되었습니다.");
            }
        }

        json res; res["status"]=Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_FORCE_CANCEL, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdAdmin::REQ_FORCE_CANCEL,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 500: 정산 목록 조회 (settlements 테이블)
// ─────────────────────────────────────────────────
void AdminHandler::handleSettlement(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req  = json::parse(body.empty()?"{}":body);
        std::string sType = req.value("type", "store"); // "store" | "rider"

        json res; res["status"]=Status::SUCCESS;

        if (sType == "store") {
            auto rows = db.executeQuery(
                "SELECT s.settlement_id, r.restaurant_name, s.period_start, "
                "s.period_end, s.net_amount, s.status "
                "FROM settlements s JOIN restaurants r ON r.restaurant_id=s.restaurant_id "
                "ORDER BY s.period_start DESC LIMIT 50");
            json list = json::array();
            for (auto& r : rows) {
                json it;
                it["id"]       = std::stoi(r["settlement_id"]);
                it["name"]     = r["restaurant_name"];
                it["from"]     = r.count("period_start")?r["period_start"]:"";
                it["to"]       = r.count("period_end")  ?r["period_end"]  :"";
                it["amount"]   = std::stoi(r.count("net_amount")?r["net_amount"]:"0");
                it["status"]   = r.count("status")?r["status"]:"PENDING";
                list.push_back(it);
            }
            res["settlements"] = list;
        } else {
            auto rows = db.executeQuery(
                "SELECT rs.settlement_id, u.name AS rider_name, rs.total_fee, rs.status "
                "FROM rider_settlements rs JOIN users u ON u.user_id=rs.rider_id "
                "ORDER BY rs.settlement_id DESC LIMIT 50");
            json list = json::array();
            for (auto& r : rows) {
                json it;
                it["id"]    = std::stoi(r["settlement_id"]);
                it["name"]  = r["rider_name"];
                it["amount"]= std::stoi(r.count("total_fee")?r["total_fee"]:"0");
                it["status"]= r.count("status")?r["status"]:"PENDING";
                list.push_back(it);
            }
            res["settlements"] = list;
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_SETTLEMENT_LIST, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdAdmin::REQ_SETTLEMENT_LIST,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 520: 사용자 제재 (리뷰 관리 포함)
// ─────────────────────────────────────────────────
void AdminHandler::handleBanUser(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        std::string action = req.value("action", "ban"); // "ban" | "del_review"

        if (action == "del_review") {
            int reviewID = req.value("review_id", 0);
            db.executeUpdate("DELETE FROM reviews WHERE review_id=" + std::to_string(reviewID));
        } else {
            int targetUID = req.value("target_user_id", 0);
            db.executeUpdate("UPDATE users SET status='SLEEP' WHERE user_id=" + std::to_string(targetUID));
        }

        json res; res["status"]=Status::SUCCESS; res["message"]="처리 완료";
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_MANAGE_REVIEW, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdAdmin::REQ_MANAGE_REVIEW,Status::SERVER_ERROR,e.what()); }
}
