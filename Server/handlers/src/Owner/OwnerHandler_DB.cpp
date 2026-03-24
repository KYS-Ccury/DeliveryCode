// OwnerHandler_DB.cpp – 실제 DB 스키마(bemin_db_create.sql) 기반 재작성
// restaurants, menu_categories, menus, option_groups, option_items,
// orders(ENUM status), order_status_logs, settlements
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
    json r; r["status"]=c; r["message"]=m;
    s->sendPacket(static_cast<uint8_t>(ClientType::OWNER), p, r.dump());
}
static std::string esc(const std::string& v) {
    std::string o; for (char c:v){if(c=='\''||c=='\\'||c=='"')o+='\\';o+=c;} return o;
}
static Session* findCustomerSession(int customerID) {
    if (!EpollServer::s_instance) return nullptr;
    return EpollServer::s_instance->getSessionByUserID(customerID);
}

void OwnerHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    switch (protocol) {
        case CmdCommon::REQ_LOGIN:           handleLogin        (session, body); break;
        case CmdOwner::REQ_STORE_INFO:       handleStoreInfo    (session, body); break;
        case CmdOwner::REQ_UPDATE_STORE:     handleUpdateStore  (session, body); break;
        case CmdOwner::REQ_ADD_MENU:         handleAddMenu      (session, body); break;
        case CmdOwner::REQ_UPDATE_MENU:      handleSoldOut      (session, body); break;
        case CmdOwner::REQ_ORDER_LIST:       handleOrderList    (session, body); break;
        case CmdOwner::REQ_ACCEPT_ORDER:     handleAcceptOrder  (session, body); break;
        case CmdOwner::REQ_REJECT_ORDER:     handleRejectOrder  (session, body); break;
        case CmdOwner::REQ_COOKING_DONE:     handleCookingDone  (session, body); break;
        case CmdOwner::REQ_SALES_STATS:      handleSalesStats   (session, body); break;
        case CmdOwner::REQ_CHANGE_STATUS:    handleChangeStatus (session, body); break;
        default: sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
    }
}

// ─────────────────────────────────────────────────
// 101: 사장님 로그인
// ─────────────────────────────────────────────────
void OwnerHandler::handleLogin(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        std::string id = esc(req.value("id","")), pw = esc(req.value("pw",""));

        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, r.restaurant_id "
            "FROM users u LEFT JOIN restaurants r ON r.owner_id = u.user_id "
            "WHERE u.login_id='" + id + "' AND u.password='" + pw + "' "
            "AND u.role='OWNER' AND u.status='ACTIVE' LIMIT 1");

        if (rows.empty()) { sendErr(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "인증 실패"); return; }

        int uid    = std::stoi(rows[0]["user_id"]);
        int restID = rows[0].count("restaurant_id") && !rows[0]["restaurant_id"].empty()
                     ? std::stoi(rows[0]["restaurant_id"]) : 0;
        session->setUserID  (uid);
        session->setUserType(static_cast<uint8_t>(ClientType::OWNER));

        json res; res["status"]=Status::SUCCESS; res["user_id"]=uid;
        res["name"]=rows[0]["name"]; res["store_id"]=restID;
        res["token"]="owtkn_"+id+"_"+std::to_string(uid);
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdCommon::REQ_LOGIN, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdCommon::REQ_LOGIN,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 305: 주문 수락 → PENDING → COOKING + 고객 Push
// ─────────────────────────────────────────────────
void OwnerHandler::handleAcceptOrder(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int ordID  = req.value("order_id", 0);
        int estMin = req.value("estimated_time", 30);

        db.executeUpdate(
            "UPDATE orders SET status='COOKING', estimated_minutes="
            + std::to_string(estMin) + " WHERE order_id=" + std::to_string(ordID)
            + " AND status='PENDING'");

        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id,from_status,to_status,changed_by) VALUES ("
            + std::to_string(ordID) + ",'PENDING','COOKING'," + std::to_string(session->getUserID()) + ")");

        // 고객 Push
        auto cRows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id="+std::to_string(ordID));
        if (!cRows.empty()) {
            int cid = std::stoi(cRows[0]["customer_id"]);
            if (auto* cs = findCustomerSession(cid))
                CustomerHandler::pushOrderStatus(cs, ordID, 2,
                    "주문이 수락되었습니다. 예상 조리 시간: " + std::to_string(estMin) + "분");
        }

        json res; res["status"]=Status::SUCCESS; res["message"]="주문 수락 완료";
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_ACCEPT_ORDER, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_ACCEPT_ORDER,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 306: 주문 거절 → CANCELED + 고객 Push
// ─────────────────────────────────────────────────
void OwnerHandler::handleRejectOrder(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int ordID = req.value("order_id", 0);
        std::string reason = esc(req.value("reason", "재료 소진"));

        db.executeUpdate(
            "UPDATE orders SET status='REJECTED', cancel_reason='" + reason
            + "', canceled_at=NOW() WHERE order_id=" + std::to_string(ordID));

        auto cRows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id="+std::to_string(ordID));
        if (!cRows.empty()) {
            int cid = std::stoi(cRows[0]["customer_id"]);
            if (auto* cs = findCustomerSession(cid))
                CustomerHandler::pushOrderStatus(cs, ordID, 0, "주문이 거절되었습니다: " + reason);
        }

        json res; res["status"]=Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_REJECT_ORDER, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_REJECT_ORDER,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 307: 조리 완료 → WAITING_PICKUP + 고객 Push
// ─────────────────────────────────────────────────
void OwnerHandler::handleCookingDone(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int ordID = req.value("order_id", 0);

        db.executeUpdate("UPDATE orders SET status='WAITING_PICKUP' WHERE order_id="+std::to_string(ordID));

        auto cRows = db.executeQuery("SELECT customer_id FROM orders WHERE order_id="+std::to_string(ordID));
        if (!cRows.empty()) {
            int cid = std::stoi(cRows[0]["customer_id"]);
            if (auto* cs = findCustomerSession(cid))
                CustomerHandler::pushOrderStatus(cs, ordID, 3, "조리가 완료되었습니다. 라이더를 기다리고 있습니다.");
        }

        json res; res["status"]=Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_COOKING_DONE, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_COOKING_DONE,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 300: 매장 정보 조회
// ─────────────────────────────────────────────────
void OwnerHandler::handleStoreInfo(Session* session, const std::string&) {
    try {
        auto& db  = MariaDBManager::getInstance();
        int   uid = session->getUserID();

        auto rows = db.executeQuery(
            "SELECT r.restaurant_id, r.restaurant_name, fc.category_name, "
            "r.address, r.phone, r.min_order_amt, r.base_delivery_fee, r.is_open, r.notice "
            "FROM restaurants r JOIN food_categories fc ON fc.category_id=r.category_id "
            "WHERE r.owner_id=" + std::to_string(uid));

        if (rows.empty()) { sendErr(session,CmdOwner::REQ_STORE_INFO,Status::NOT_FOUND,"매장 없음"); return; }
        auto& r = rows[0];
        json res; res["status"]=Status::SUCCESS;
        res["id"]           = std::stoi(r["restaurant_id"]);
        res["name"]         = r["restaurant_name"];
        res["category"]     = r["category_name"];
        res["address"]      = r.count("address")          ? r["address"]          : "";
        res["phone"]        = r.count("phone")            ? r["phone"]            : "";
        res["min_order"]    = std::stoi(r.count("min_order_amt")     ? r["min_order_amt"]     : "0");
        res["delivery_fee"] = std::stoi(r.count("base_delivery_fee") ? r["base_delivery_fee"] : "0");
        res["is_open"]      = (r.count("is_open") && r["is_open"]=="1");
        res["notice"]       = r.count("notice") ? r["notice"] : "";
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_STORE_INFO, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_STORE_INFO,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 301: 매장 정보 수정
// ─────────────────────────────────────────────────
void OwnerHandler::handleUpdateStore(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int restID = req.value("store_id", 0);
        std::string q = "UPDATE restaurants SET ";
        bool first = true;
        auto field = [&](const char* k, const char* col) {
            if (!req.contains(k)) return;
            if (!first) q+=","; first=false;
            q+=std::string(col)+"='"+esc(req.value(k,""))+"'";
        };
        field("name","restaurant_name"); field("address","address");
        field("phone","phone"); field("notice","notice");
        if (req.contains("min_order"))
            q+=(first?"":",")+"min_order_amt="+std::to_string(req.value("min_order",0)), first=false;
        q+=" WHERE restaurant_id="+std::to_string(restID);
        db.executeUpdate(q);
        json res; res["status"]=Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_UPDATE_STORE, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_UPDATE_STORE,Status::SERVER_ERROR,e.what()); }
}

// ─────────────────────────────────────────────────
// 302: 메뉴 추가 (menu_categories 연동)
// ─────────────────────────────────────────────────
void OwnerHandler::handleAddMenu(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int restID   = req.value("store_id",   0);
        std::string subCat = esc(req.value("sub_category", "기타"));

        // sub_category 없으면 자동 생성
        auto catRows = db.executeQuery(
            "SELECT menu_category_id FROM menu_categories "
            "WHERE restaurant_id=" + std::to_string(restID)
            + " AND category_name='" + subCat + "' LIMIT 1");
        int catID = 0;
        if (catRows.empty()) {
            db.executeUpdate(
                "INSERT INTO menu_categories (restaurant_id, category_name) VALUES ("
                + std::to_string(restID) + ",'" + subCat + "')");
            catID = (int)db.getLastInsertId();
        } else {
            catID = std::stoi(catRows[0]["menu_category_id"]);
        }

        db.executeUpdate(
            "INSERT INTO menus (menu_category_id, menu_name, price, description) VALUES ("
            + std::to_string(catID) + ",'" + esc(req.value("name","")) + "',"
            + std::to_string(req.value("price",0)) + ",'" + esc(req.value("desc","")) + "')");
        uint64_t menuID = db.getLastInsertId();

        // 옵션 그룹/아이템
        if (req.contains("options") && !req["options"].empty()) {
            db.executeUpdate(
                "INSERT INTO option_groups (menu_id, group_name, is_essential) VALUES ("
                + std::to_string(menuID) + ",'선택 옵션',0)");
            uint64_t grpID = db.getLastInsertId();
            for (auto& opt : req["options"]) {
                db.executeUpdate(
                    "INSERT INTO option_items (option_group_id, option_name, extra_price) VALUES ("
                    + std::to_string(grpID) + ",'" + esc(opt.value("name","")) + "',"
                    + std::to_string(opt.value("price",0)) + ")");
            }
        }

        json res; res["status"]=Status::SUCCESS; res["menu_id"]=(int)menuID;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_ADD_MENU, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_ADD_MENU,Status::SERVER_ERROR,e.what()); }
}

// 303: 품절 처리
void OwnerHandler::handleSoldOut(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int menuID = req.value("menu_id", 0);
        int sold   = req.value("is_sold_out", 1); // 1=품절
        db.executeUpdate("UPDATE menus SET is_sold_out="+std::to_string(sold)+" WHERE menu_id="+std::to_string(menuID));
        json res; res["status"]=Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_UPDATE_MENU, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_UPDATE_MENU,Status::SERVER_ERROR,e.what()); }
}

// 304: 주문 목록
void OwnerHandler::handleOrderList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int restID = req.value("store_id", 0);
        std::string listType = req.value("list_type", "waiting");

        std::string statusFilter;
        if      (listType=="waiting") statusFilter="o.status='PENDING'";
        else if (listType=="active")  statusFilter="o.status IN ('COOKING','WAITING_PICKUP','DELIVERING')";
        else                          statusFilter="o.status='DONE'";

        auto rows = db.executeQuery(
            "SELECT o.order_id, u.name AS cust_name, u.phone AS cust_phone, "
            "o.total_price, o.delivery_method, o.created_at, o.estimated_minutes "
            "FROM orders o JOIN users u ON u.user_id=o.customer_id "
            "WHERE o.restaurant_id=" + std::to_string(restID)
            + " AND " + statusFilter + " ORDER BY o.created_at DESC");

        json orders = json::array();
        for (auto& r : rows) {
            json o;
            o["order_id"]       = std::stoi(r["order_id"]);
            o["cust_name"]      = r["cust_name"];
            o["cust_phone"]     = r.count("cust_phone")      ? r["cust_phone"]      : "";
            o["total_price"]    = std::stoi(r.count("total_price")   ? r["total_price"]   : "0");
            o["delivery_method"]= r.count("delivery_method") ? r["delivery_method"] : "";
            o["order_time"]     = r.count("created_at")      ? r["created_at"]      : "";
            o["estimated_time"] = r.count("estimated_minutes")? r["estimated_minutes"]:"0";
            orders.push_back(o);
        }
        json res; res["status"]=Status::SUCCESS; res["orders"]=orders;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_ORDER_LIST, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_ORDER_LIST,Status::SERVER_ERROR,e.what()); }
}

// 308: 매출 통계
void OwnerHandler::handleSalesStats(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int restID = req.value("store_id", 0);
        std::string from = esc(req.value("from","")), to = esc(req.value("to",""));

        std::string q =
            "SELECT DATE(created_at) AS day, COUNT(*) AS cnt, SUM(total_price) AS total "
            "FROM orders WHERE restaurant_id=" + std::to_string(restID)
            + " AND status='DONE'";
        if (!from.empty()) q+=" AND created_at>='"+from+"'";
        if (!to.empty())   q+=" AND created_at<='"+to+"'";
        q+=" GROUP BY DATE(created_at) ORDER BY day DESC";

        auto rows = db.executeQuery(q);
        json stats = json::array();
        for (auto& r : rows) {
            json s; s["day"]=r["day"];
            s["count"]=std::stoi(r.count("cnt")  ?r["cnt"]  :"0");
            s["total"]=std::stoi(r.count("total")?r["total"]:"0");
            stats.push_back(s);
        }
        json res; res["status"]=Status::SUCCESS; res["stats"]=stats;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_SALES_STATS, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_SALES_STATS,Status::SERVER_ERROR,e.what()); }
}

// 309: 영업 상태 변경
void OwnerHandler::handleChangeStatus(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int restID  = req.value("store_id",  0);
        int isOpen  = req.value("is_active", 1);
        db.executeUpdate("UPDATE restaurants SET is_open="+std::to_string(isOpen)+" WHERE restaurant_id="+std::to_string(restID));
        json res; res["status"]=Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::OWNER), CmdOwner::REQ_CHANGE_STATUS, res.dump());
    } catch(const std::exception& e){ sendErr(session,CmdOwner::REQ_CHANGE_STATUS,Status::SERVER_ERROR,e.what()); }
}
