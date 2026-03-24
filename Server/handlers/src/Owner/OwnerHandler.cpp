// OwnerHandler.cpp v2 – bemin_db 실제 스키마 기준 완성본
// 주요 테이블: users, restaurants, food_categories, menu_categories,
//             menus, option_groups, option_items,
//             orders, order_items, order_status_logs, settlements
#include "OwnerHandler.h"
#include "CustomerHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include "EpollServer.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

static void sendErr(Session* s, uint16_t p, uint16_t c, const std::string& m) {
    json r; r["status"] = c; r["message"] = m;
    s->sendPacket(static_cast<uint8_t>(ClientType::OWNER), p, r.dump());
}
static std::string esc(const std::string& s) {
    std::string o; for (char c : s) { if(c=='\''||c=='\\'||c=='"') o+='\\'; o+=c; } return o;
}

void OwnerHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    switch (protocol) {
        case CmdCommon::REQ_LOGIN:            handleLogin        (session, body); break;
        case CmdOwner::REQ_STORE_INFO:        handleStoreInfo    (session, body); break;
        case CmdOwner::REQ_UPDATE_STORE:      handleUpdateStore  (session, body); break;
        case CmdOwner::REQ_ADD_MENU:          handleAddMenu      (session, body); break;
        case CmdOwner::REQ_UPDATE_MENU:       handleSoldOut      (session, body); break;
        case CmdOwner::REQ_ORDER_LIST:        handleOrderList    (session, body); break;
        case CmdOwner::REQ_ACCEPT_ORDER:      handleAcceptOrder  (session, body); break;
        case CmdOwner::REQ_REJECT_ORDER:      handleRejectOrder  (session, body); break;
        case CmdOwner::REQ_COOKING_DONE:      handleCookingDone  (session, body); break;
        case CmdOwner::REQ_SALES_STATS:       handleSalesStats   (session, body); break;
        case CmdOwner::REQ_CHANGE_STATUS:     handleChangeStatus (session, body); break;
        default: sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol"); break;
    }
}

// 고객 세션 조회 헬퍼
static Session* findCustomer(int customerID) {
    return EpollServer::s_instance ? EpollServer::s_instance->getSessionByUserID(customerID) : nullptr;
}

// ──────────────────────────────────────────────────────
// 101: 사장님 로그인
// ──────────────────────────────────────────────────────
void OwnerHandler::handleLogin(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        std::string id = esc(req.value("id",""));
        std::string pw = esc(req.value("pw",""));

        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, r.restaurant_id "
            "FROM users u LEFT JOIN restaurants r ON r.owner_id = u.user_id "
            "WHERE u.login_id='" + id + "' AND u.password='" + pw
            + "' AND u.role='OWNER' AND u.status='ACTIVE' LIMIT 1");

        if (rows.empty()) { sendErr(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "로그인 실패"); return; }

        int uid    = std::stoi(rows[0]["user_id"]);
        int restId = rows[0].count("restaurant_id") && !rows[0]["restaurant_id"].empty()
                     ? std::stoi(rows[0]["restaurant_id"]) : 0;
        session->setUserID(uid);
        session->setUserType(static_cast<uint8_t>(ClientType::OWNER));

        json res; res["status"] = Status::SUCCESS;
        res["user_id"]   = uid;
        res["name"]      = rows[0]["name"];
        res["store_id"]  = restId;
        res["token"]     = "owntkn_" + std::to_string(uid);
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdCommon::REQ_LOGIN, res.dump());

    } catch (const std::exception& e) { sendErr(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 300: 매장 정보 조회
// ──────────────────────────────────────────────────────
void OwnerHandler::handleStoreInfo(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        int uid  = session->getUserID();
        auto rows = db.executeQuery(
            "SELECT r.restaurant_id, r.restaurant_name AS name, fc.category_name AS category, "
            "       r.address, r.phone, r.min_order_amt, r.base_delivery_fee, "
            "       r.is_open, r.rating_avg, r.notice "
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.owner_id=" + std::to_string(uid));

        if (rows.empty()) { sendErr(session, CmdOwner::REQ_STORE_INFO, Status::NOT_FOUND, "매장 없음"); return; }
        auto& r = rows[0];
        json res; res["status"] = Status::SUCCESS;
        res["id"]          = std::stoi(r["restaurant_id"]);
        res["name"]        = r["name"];
        res["category"]    = r["category"];
        res["address"]     = r.count("address") ? r["address"] : "";
        res["phone"]       = r.count("phone")   ? r["phone"]   : "";
        res["min_order"]   = r.count("min_order_amt")      && !r["min_order_amt"].empty()      ? std::stoi(r["min_order_amt"])      : 0;
        res["delivery_fee"]= r.count("base_delivery_fee")  && !r["base_delivery_fee"].empty()  ? std::stoi(r["base_delivery_fee"])  : 0;
        res["is_open"]     = (r.count("is_open") && r["is_open"] == "1");
        res["rating"]      = r.count("rating_avg") && !r["rating_avg"].empty() ? std::stod(r["rating_avg"]) : 0.0;
        res["notice"]      = r.count("notice") ? r["notice"] : "";
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_STORE_INFO, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_STORE_INFO, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 301: 매장 정보 수정
// ──────────────────────────────────────────────────────
void OwnerHandler::handleUpdateStore(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restId = req.value("store_id", 0);

        std::string q = "UPDATE restaurants SET ";
        bool first = true;
        auto addStr = [&](const char* key, const char* col) {
            if (!req.contains(key)) return;
            if (!first) q += ","; first = false;
            q += std::string(col) + "='" + esc(req[key].get<std::string>()) + "'";
        };
        auto addInt = [&](const char* key, const char* col) {
            if (!req.contains(key)) return;
            if (!first) q += ","; first = false;
            q += std::string(col) + "=" + std::to_string(req[key].get<int>());
        };
        addStr("name",         "restaurant_name");
        addStr("address",      "address");
        addStr("phone",        "phone");
        addStr("notice",       "notice");
        addInt("min_order",    "min_order_amt");
        addInt("delivery_fee", "base_delivery_fee");
        q += " WHERE restaurant_id=" + std::to_string(restId);

        db.executeUpdate(q);
        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_UPDATE_STORE, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_UPDATE_STORE, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 302: 메뉴 등록
// 요청: { store_id, sub_category(menu_category_id or name), name, desc, price,
//         options:[{group_name, is_essential, items:[{name,extra_price}]}] }
// ──────────────────────────────────────────────────────
void OwnerHandler::handleAddMenu(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restId  = req.value("store_id", 0);
        std::string catName = esc(req.value("sub_category", "기본메뉴"));

        // menu_categories 조회 또는 생성
        auto catRows = db.executeQuery(
            "SELECT menu_category_id FROM menu_categories "
            "WHERE restaurant_id=" + std::to_string(restId)
            + " AND category_name='" + catName + "' LIMIT 1");
        uint64_t catId;
        if (catRows.empty()) {
            db.executeUpdate("INSERT INTO menu_categories (restaurant_id,category_name) VALUES ("
                             + std::to_string(restId) + ",'" + catName + "')");
            catId = db.getLastInsertId();
        } else {
            catId = std::stoi(catRows[0]["menu_category_id"]);
        }

        // menus INSERT
        db.executeUpdate(
            "INSERT INTO menus (menu_category_id,menu_name,price,description) VALUES ("
            + std::to_string(catId) + ",'"
            + esc(req.value("name","")) + "',"
            + std::to_string(req.value("price", 0)) + ",'"
            + esc(req.value("desc","")) + "')");
        uint64_t menuId = db.getLastInsertId();

        // option_groups + option_items
        if (req.contains("options")) {
            for (auto& og : req["options"]) {
                std::string gName  = esc(og.value("group_name",   ""));
                bool isEssential   = og.value("is_essential", false);
                int  maxSel        = og.value("max_select",    1);
                db.executeUpdate(
                    "INSERT INTO option_groups (menu_id,group_name,is_essential,max_select) VALUES ("
                    + std::to_string(menuId) + ",'" + gName + "',"
                    + (isEssential ? "1" : "0") + "," + std::to_string(maxSel) + ")");
                uint64_t ogId = db.getLastInsertId();
                if (og.contains("items")) {
                    for (auto& oi : og["items"]) {
                        db.executeUpdate(
                            "INSERT INTO option_items (option_group_id,option_name,extra_price) VALUES ("
                            + std::to_string(ogId) + ",'"
                            + esc(oi.value("name","")) + "',"
                            + std::to_string(oi.value("extra_price", 0)) + ")");
                    }
                }
            }
        }
        json res; res["status"] = Status::SUCCESS; res["menu_id"] = (int)menuId;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_ADD_MENU, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_ADD_MENU, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 303: 품절/판매중 전환 (menus.is_sold_out)
// ──────────────────────────────────────────────────────
void OwnerHandler::handleSoldOut(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int menuId   = req.value("menu_id",      0);
        int isSoldOut= req.value("is_sold_out",  1); // 1=품절, 0=판매중
        db.executeUpdate("UPDATE menus SET is_sold_out=" + std::to_string(isSoldOut)
                         + " WHERE menu_id=" + std::to_string(menuId));
        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_UPDATE_MENU, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_UPDATE_MENU, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 304: 주문 목록 조회
// ──────────────────────────────────────────────────────
void OwnerHandler::handleOrderList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restId   = req.value("store_id",  0);
        std::string lt = req.value("list_type", "pending"); // pending|active|done

        std::string statusFilter;
        if      (lt == "pending") statusFilter = "status='PENDING'";
        else if (lt == "active")  statusFilter = "status IN ('ACCEPTED','COOKING','WAITING_PICKUP','DELIVERING')";
        else                      statusFilter = "status='DONE'";

        auto rows = db.executeQuery(
            "SELECT o.order_id, u.name AS cust_name, u.phone AS cust_phone, "
            "       o.total_price, o.delivery_method, o.status, o.created_at AS order_time, "
            "       o.estimated_minutes "
            "FROM orders o JOIN users u ON u.user_id = o.customer_id "
            "WHERE o.restaurant_id=" + std::to_string(restId)
            + " AND " + statusFilter + " ORDER BY o.created_at DESC");

        json orders = json::array();
        for (auto& r : rows) {
            json o;
            o["order_id"]       = std::stoi(r["order_id"]);
            o["cust_name"]      = r["cust_name"];
            o["cust_phone"]     = r.count("cust_phone")  ? r["cust_phone"]  : "";
            o["total_price"]    = r.count("total_price") && !r["total_price"].empty() ? std::stoi(r["total_price"]) : 0;
            o["delivery_method"]= r.count("delivery_method") ? r["delivery_method"] : "DELIVERY";
            o["status"]         = r["status"];
            o["order_time"]     = r.count("order_time")  ? r["order_time"]  : "";
            o["estimated_min"]  = r.count("estimated_minutes") && !r["estimated_minutes"].empty()
                                  ? std::stoi(r["estimated_minutes"]) : 0;
            orders.push_back(o);
        }
        json res; res["status"] = Status::SUCCESS; res["orders"] = orders;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_ORDER_LIST, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_ORDER_LIST, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 305: 주문 수락 → PENDING → ACCEPTED, 고객 Push
// 요청: { order_id, estimated_time: 10|20|30|40|50|60 }
// ──────────────────────────────────────────────────────
void OwnerHandler::handleAcceptOrder(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int orderId  = req.value("order_id",       0);
        int estMin   = req.value("estimated_time", 30);

        db.executeUpdate(
            "UPDATE orders SET status='ACCEPTED',estimated_minutes=" + std::to_string(estMin)
            + " WHERE order_id=" + std::to_string(orderId)
            + " AND status='PENDING'");
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id,from_status,to_status,changed_by) VALUES ("
            + std::to_string(orderId) + ",'PENDING','ACCEPTED'," + std::to_string(session->getUserID()) + ")");

        // 고객 Push
        auto rows = db.executeQuery(
            "SELECT customer_id FROM orders WHERE order_id=" + std::to_string(orderId));
        if (!rows.empty()) {
            Session* cs = findCustomer(std::stoi(rows[0]["customer_id"]));
            if (cs) CustomerHandler::pushOrderStatus(cs, orderId, 2,
                "주문이 수락되었습니다. 예상 조리 시간: " + std::to_string(estMin) + "분");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_ACCEPT_ORDER, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_ACCEPT_ORDER, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 306: 주문 거절 → PENDING → REJECTED, 고객 Push
// ──────────────────────────────────────────────────────
void OwnerHandler::handleRejectOrder(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   orderId = req.value("order_id", 0);
        std::string reason = esc(req.value("reason", "재료 소진"));

        db.executeUpdate(
            "UPDATE orders SET status='REJECTED',cancel_reason='" + reason
            + "',canceled_at=NOW() WHERE order_id=" + std::to_string(orderId));
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id,from_status,to_status,reason,changed_by) VALUES ("
            + std::to_string(orderId) + ",'PENDING','REJECTED','" + reason + "',"
            + std::to_string(session->getUserID()) + ")");

        auto rows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id=" + std::to_string(orderId));
        if (!rows.empty()) {
            Session* cs = findCustomer(std::stoi(rows[0]["customer_id"]));
            if (cs) CustomerHandler::pushOrderStatus(cs, orderId, 4, "주문이 거절되었습니다: " + reason);
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_REJECT_ORDER, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_REJECT_ORDER, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 307: 조리 완료 → ACCEPTED/COOKING → WAITING_PICKUP, 고객 Push
// ──────────────────────────────────────────────────────
void OwnerHandler::handleCookingDone(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   orderId = req.value("order_id", 0);

        db.executeUpdate(
            "UPDATE orders SET status='WAITING_PICKUP' WHERE order_id=" + std::to_string(orderId));
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id,from_status,to_status,changed_by) VALUES ("
            + std::to_string(orderId) + ",'COOKING','WAITING_PICKUP'," + std::to_string(session->getUserID()) + ")");

        auto rows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id=" + std::to_string(orderId));
        if (!rows.empty()) {
            Session* cs = findCustomer(std::stoi(rows[0]["customer_id"]));
            if (cs) CustomerHandler::pushOrderStatus(cs, orderId, 3, "조리가 완료되어 라이더를 기다리고 있습니다.");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_COOKING_DONE, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_COOKING_DONE, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 308: 매출 통계 조회 (settlements + orders)
// ──────────────────────────────────────────────────────
void OwnerHandler::handleSalesStats(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restId = req.value("store_id", 0);
        std::string from = req.value("from", "");
        std::string to   = req.value("to",   "");

        std::string q =
            "SELECT DATE(o.created_at) AS day, COUNT(*) AS cnt, SUM(o.total_price) AS total "
            "FROM orders o "
            "WHERE o.restaurant_id=" + std::to_string(restId)
            + " AND o.status='DONE'";
        if (!from.empty()) q += " AND o.created_at>='" + esc(from) + "'";
        if (!to.empty())   q += " AND o.created_at<='" + esc(to)   + "'";
        q += " GROUP BY DATE(o.created_at) ORDER BY day DESC";

        auto rows = db.executeQuery(q);
        json stats = json::array();
        for (auto& r : rows) {
            json s;
            s["day"]   = r["day"];
            s["count"] = r.count("cnt")   && !r["cnt"].empty()   ? std::stoi(r["cnt"])   : 0;
            s["total"] = r.count("total") && !r["total"].empty() ? std::stoi(r["total"]) : 0;
            stats.push_back(s);
        }
        json res; res["status"] = Status::SUCCESS; res["stats"] = stats;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_SALES_STATS, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_SALES_STATS, Status::SERVER_ERROR, e.what()); }
}

// ──────────────────────────────────────────────────────
// 309: 영업 상태 변경 (restaurants.is_open)
// ──────────────────────────────────────────────────────
void OwnerHandler::handleChangeStatus(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int restId  = req.value("store_id",  0);
        int isOpen  = req.value("is_active", 1);
        db.executeUpdate("UPDATE restaurants SET is_open=" + std::to_string(isOpen)
                         + " WHERE restaurant_id=" + std::to_string(restId));
        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_CHANGE_STATUS, res.dump());
    } catch (const std::exception& e) { sendErr(session, CmdOwner::REQ_CHANGE_STATUS, Status::SERVER_ERROR, e.what()); }
}
