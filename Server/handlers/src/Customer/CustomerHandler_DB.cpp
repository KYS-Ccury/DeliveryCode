// CustomerHandler_DB.cpp – 실제 DB 스키마(bemin_db_create.sql) 기반 완전 재작성
//
// ★ 핵심 스키마 변경 사항 (기존 구현과 차이점):
//   - users.user_id (not id), users.role = ENUM('CUSTOMER','OWNER','RIDER','ADMIN')
//   - restaurants (not stores): restaurant_id, restaurant_name, owner_id, rating_avg, base_delivery_fee, is_open
//   - menus.menu_category_id → menu_categories.restaurant_id 로 restaurant 연결
//   - option_groups / option_items (not menu_options)
//   - orders.status = ENUM('PENDING','ACCEPTED','REJECTED','COOKING','WAITING_PICKUP','DELIVERING','DONE','CANCELED')
//   - orders.restaurant_id (not store_id), orders.customer_id, orders.total_price
//   - order_items: menu_name(스냅샷), price_at_order(스냅샷), quantity
//   - order_item_options: option_name(스냅샷), extra_price(스냅샷)
//   - reviews: review_id, order_id(UNIQUE), customer_id, rating, content
//   - customer_profiles.point (not users.point)
//   - chat_rooms: room_id, order_id, room_type
//   - chat_messages: message_id, room_id, sender_id, content, sent_at
//   - payments: payment_id, order_id, method_type, total_amount, status

#include "CustomerHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

// ─────────────────────────────────────────────────
// 헬퍼
// ─────────────────────────────────────────────────
static void sendErr(Session* s, uint16_t proto, uint16_t code, const std::string& msg) {
    json r; r["status"] = code; r["message"] = msg;
    s->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), proto, r.dump());
}
static std::string esc(const std::string& v) {
    std::string o; o.reserve(v.size()*2);
    for (char c : v) { if (c=='\'' || c=='\\' || c=='"') o+='\\'; o+=c; }
    return o;
}
// orders.status ENUM → 숫자 (클라이언트 호환)
static int statusToInt(const std::string& s) {
    if (s=="CANCELED")       return 0;
    if (s=="PENDING")        return 1;
    if (s=="ACCEPTED"||s=="COOKING") return 2;
    if (s=="WAITING_PICKUP") return 3;
    if (s=="DELIVERING")     return 3;
    if (s=="DONE")           return 4;
    return 0;
}
static std::string intToStatus(int v) {
    switch(v) {
        case 0: return "CANCELED";
        case 1: return "PENDING";
        case 2: return "COOKING";
        case 3: return "DELIVERING";
        case 4: return "DONE";
        default: return "PENDING";
    }
}

// ─────────────────────────────────────────────────
// Dispatcher
// ─────────────────────────────────────────────────
void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    switch (protocol) {
        case CmdCommon::REQ_SIGNUP:          handleSignup      (session, body); break;
        case CmdCommon::REQ_LOGIN:           handleLogin       (session, body); break;
        case CmdCommon::REQ_LOGOUT:          handleLogout      (session, body); break;
        case CmdCommon::REQ_GET_PROFILE:     handleGetProfile  (session, body); break;
        case CmdCustomer::REQ_STORE_LIST:    handleStoreList   (session, body); break;
        case CmdCustomer::REQ_MENU_LIST:     handleMenuList    (session, body); break;
        case CmdCustomer::REQ_CREATE_ORDER:  handleCreateOrder (session, body); break;
        case CmdCustomer::REQ_ORDER_HISTORY: handleOrderHistory(session, body); break;
        case CmdCustomer::REQ_ORDER_DETAIL:  handleOrderDetail (session, body); break;
        case CmdCustomer::REQ_PAYMENT:       handlePayment     (session, body); break;
        case CmdCustomer::REQ_WRITE_REVIEW:  handleWriteReview (session, body); break;
        case CmdCustomer::REQ_REVIEW_LIST:   handleReviewList  (session, body); break;
        case CmdCustomer::REQ_CANCEL_ORDER:  handleCancelOrder (session, body); break;
        default:
            sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
    }
}

// ─────────────────────────────────────────────────
// 100: 회원가입
// 요청: { id, pw, name, phone, address, role(선택: "CUSTOMER") }
// ─────────────────────────────────────────────────
void CustomerHandler::handleSignup(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);

        std::string loginId = esc(req.value("id",      ""));
        std::string pw      = esc(req.value("pw",      ""));
        std::string name    = esc(req.value("name",    "이름없음"));
        std::string phone   = esc(req.value("phone",   ""));
        std::string address = esc(req.value("address", ""));
        std::string role    = "CUSTOMER"; // 고객 클라이언트는 항상 CUSTOMER

        if (loginId.empty() || pw.empty()) {
            sendErr(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "ID/PW 필수");
            return;
        }

        // 중복 확인
        auto chk = db.executeQuery("SELECT user_id FROM users WHERE login_id='" + loginId + "'");
        if (!chk.empty()) {
            sendErr(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "이미 사용 중인 아이디입니다.");
            return;
        }

        // users INSERT
        db.executeUpdate(
            "INSERT INTO users (login_id,password,role,name,phone,address,status) VALUES ('"
            + loginId + "','" + pw + "','CUSTOMER','" + name + "','" + phone + "','" + address + "','ACTIVE')");

        uint64_t uid = db.getLastInsertId();

        // customer_profiles INSERT (포인트 0으로 초기화)
        db.executeUpdate(
            "INSERT INTO customer_profiles (user_id, point) VALUES (" + std::to_string(uid) + ",0)");

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_SIGNUP, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 101: 로그인
// 응답: { status, token, user_id, name, address, point }
// ─────────────────────────────────────────────────
void CustomerHandler::handleLogin(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);

        std::string loginId = esc(req.value("id", ""));
        std::string pw      = esc(req.value("pw", ""));

        // users + customer_profiles JOIN
        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, u.address, COALESCE(cp.point,0) AS point "
            "FROM users u "
            "LEFT JOIN customer_profiles cp ON cp.user_id = u.user_id "
            "WHERE u.login_id='" + loginId + "' AND u.password='" + pw + "' "
            "AND u.role='CUSTOMER' AND u.status='ACTIVE' LIMIT 1");

        if (rows.empty()) {
            sendErr(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "아이디 또는 비밀번호가 틀렸습니다.");
            return;
        }

        int uid = std::stoi(rows[0]["user_id"]);
        session->setUserID  (uid);
        session->setUserType(static_cast<uint8_t>(ClientType::CUSTOMER));

        json res;
        res["status"]  = Status::SUCCESS;
        res["token"]   = "ctkn_" + loginId + "_" + std::to_string(uid);
        res["user_id"] = uid;
        res["name"]    = rows[0]["name"];
        res["address"] = rows[0]["address"];
        res["point"]   = std::stoi(rows[0]["point"]);
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_LOGIN, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 102: 로그아웃
// ─────────────────────────────────────────────────
void CustomerHandler::handleLogout(Session* session, const std::string&) {
    session->setUserID(0);
    json res; res["status"] = Status::SUCCESS;
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_LOGOUT, res.dump());
}

// ─────────────────────────────────────────────────
// 104: 프로필 조회 / 수정 / 카드 등록
// ─────────────────────────────────────────────────
void CustomerHandler::handleGetProfile(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        int uid = session->getUserID();
        if (uid == 0) { sendErr(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "로그인 필요"); return; }

        json req = json::parse(body);

        // 주소 변경 요청
        if (req.contains("address")) {
            std::string addr = esc(req.value("address",""));
            db.executeUpdate("UPDATE users SET address='" + addr + "' WHERE user_id=" + std::to_string(uid));
        }

        // 카드 등록 요청
        if (req.contains("card_number")) {
            std::string alias   = esc(req.value("card_name",    "내 카드"));
            std::string masked  = esc(req.value("card_number",  ""));
            // 마스킹: 마지막 4자리만 보관
            if (masked.size() >= 4)
                masked = "****-****-****-" + masked.substr(masked.size()-4);
            db.executeUpdate(
                "INSERT INTO payment_methods (user_id, method_type, card_alias, card_num_masked, is_default) "
                "VALUES (" + std::to_string(uid) + ",'CARD','" + alias + "','" + masked + "',0)");
        }

        auto rows = db.executeQuery(
            "SELECT u.name, u.address, u.phone, COALESCE(cp.point,0) AS point "
            "FROM users u LEFT JOIN customer_profiles cp ON cp.user_id=u.user_id "
            "WHERE u.user_id=" + std::to_string(uid));

        json res; res["status"] = Status::SUCCESS;
        if (!rows.empty()) {
            res["name"]    = rows[0]["name"];
            res["address"] = rows[0]["address"];
            res["phone"]   = rows[0]["phone"];
            res["point"]   = std::stoi(rows[0]["point"]);
        }
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_GET_PROFILE, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 200: 음식점(restaurants) 목록 조회
// 요청: { category("전체"|"한식"|...) }
// ─────────────────────────────────────────────────
void CustomerHandler::handleStoreList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body.empty() ? "{}" : body);
        std::string category = req.value("category", "전체");

        std::string q =
            "SELECT r.restaurant_id, r.restaurant_name, fc.category_name, "
            "r.base_delivery_fee, r.min_order_amt, r.rating_avg, r.address, "
            "r.phone, r.is_open "
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.is_open = TRUE ";
        if (category != "전체")
            q += "AND fc.category_name='" + esc(category) + "' ";
        q += "ORDER BY r.rating_avg DESC, r.dib_count DESC";

        auto rows = db.executeQuery(q);
        json stores = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]           = std::stoi(r["restaurant_id"]);
            s["name"]         = r["restaurant_name"];
            s["category"]     = r["category_name"];
            s["delivery_fee"] = std::stoi(r.count("base_delivery_fee") ? r["base_delivery_fee"] : "0");
            s["min_order"]    = std::stoi(r.count("min_order_amt")     ? r["min_order_amt"]     : "0");
            s["rating"]       = r.count("rating_avg") ? std::stod(r["rating_avg"]) : 0.0;
            s["address"]      = r.count("address")    ? r["address"]    : "";
            s["phone"]        = r.count("phone")      ? r["phone"]      : "";
            s["delivery_time"]= "20~40분"; // 실시간 추정치 (라이더 위치 기반 확장 가능)
            s["distance"]     = 0.0;       // GPS 기반 확장 가능
            stores.push_back(s);
        }

        json res; res["status"] = Status::SUCCESS; res["stores"] = stores;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_STORE_LIST, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_STORE_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 201: 메뉴 목록 조회
// 요청: { restaurant_id(=store_id), sub_category }
// 응답: { menus: [{id, name, desc, price, sub_category, options:[{name,price}]}] }
// ─────────────────────────────────────────────────
void CustomerHandler::handleMenuList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int restID   = req.value("store_id", 0);
        std::string sub = esc(req.value("sub_category", "전체"));

        if (restID == 0) { sendErr(session, CmdCustomer::REQ_MENU_LIST, Status::BAD_REQUEST, "restaurant_id 필수"); return; }

        // menu_categories → menus JOIN
        std::string q =
            "SELECT m.menu_id, m.menu_name, m.description, m.price, "
            "mc.category_name AS sub_cat, m.is_sold_out "
            "FROM menus m "
            "JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
            "WHERE mc.restaurant_id=" + std::to_string(restID);
        if (sub != "전체")
            q += " AND mc.category_name='" + sub + "'";
        q += " ORDER BY mc.sort_order, m.menu_id";

        auto rows = db.executeQuery(q);
        json menus = json::array();

        for (auto& r : rows) {
            if (r.count("is_sold_out") && r["is_sold_out"] == "1") continue; // 품절 제외

            int menuID = std::stoi(r["menu_id"]);
            json m;
            m["id"]           = menuID;
            m["name"]         = r["menu_name"];
            m["desc"]         = r.count("description") ? r["description"] : "";
            m["price"]        = std::stoi(r.count("price") ? r["price"] : "0");
            m["sub_category"] = r.count("sub_cat")    ? r["sub_cat"]    : "";

            // option_groups → option_items
            auto grpRows = db.executeQuery(
                "SELECT og.option_group_id, og.group_name, og.is_essential "
                "FROM option_groups og WHERE og.menu_id=" + std::to_string(menuID));

            json opts = json::array();
            for (auto& grp : grpRows) {
                int grpID = std::stoi(grp["option_group_id"]);
                auto itemRows = db.executeQuery(
                    "SELECT option_name, extra_price FROM option_items "
                    "WHERE option_group_id=" + std::to_string(grpID));
                for (auto& oi : itemRows) {
                    json opt;
                    opt["name"]  = oi["option_name"];
                    opt["price"] = std::stoi(oi.count("extra_price") ? oi["extra_price"] : "0");
                    opts.push_back(opt);
                }
            }
            m["options"] = opts;
            menus.push_back(m);
        }

        json res; res["status"] = Status::SUCCESS; res["menus"] = menus;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_MENU_LIST, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_MENU_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 202: 주문 생성 (+ payments INSERT)
// 요청: { store_id, items:[{menu_id,qty,price,name,options:[{name,price}]}],
//          is_delivery, delivery_address, use_point, coupon_id }
// ─────────────────────────────────────────────────
void CustomerHandler::handleCreateOrder(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   uid = session->getUserID();

        int  restID  = req.value("store_id",   0);
        bool isDel   = req.value("is_delivery", true);
        std::string delAddr = esc(req.value("delivery_address", ""));
        int  usePoint= req.value("use_point",  0);

        if (uid == 0 || restID == 0) {
            sendErr(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "로그인 및 restaurant_id 필수");
            return;
        }

        // 총 금액 계산
        int totalPrice = 0;
        for (auto& it : req["items"]) {
            int unitPrice = it.value("price", 0);
            int qty       = it.value("qty",   1);
            // 옵션 추가금
            if (it.contains("options"))
                for (auto& opt : it["options"])
                    unitPrice += opt.value("price", 0);
            totalPrice += unitPrice * qty;
        }

        // 포인트 사용
        if (usePoint > 0) {
            auto ptRow = db.executeQuery(
                "SELECT point FROM customer_profiles WHERE user_id=" + std::to_string(uid));
            if (!ptRow.empty()) {
                int avail = std::stoi(ptRow[0]["point"]);
                if (usePoint > avail) usePoint = avail; // 보유 포인트 초과 방지
            }
            totalPrice -= usePoint;
        }
        if (totalPrice < 0) totalPrice = 0;

        std::string delMethod = isDel ? "배달" : "포장";
        std::string delAddrVal = isDel ? "'" + delAddr + "'" : "NULL";

        // orders INSERT
        std::string insOrd =
            "INSERT INTO orders (customer_id, restaurant_id, status, total_price, "
            "delivery_method, delivery_address) VALUES ("
            + std::to_string(uid) + "," + std::to_string(restID)
            + ",'PENDING'," + std::to_string(totalPrice)
            + ",'" + delMethod + "'," + delAddrVal + ")";
        if (!db.executeUpdate(insOrd)) {
            sendErr(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, "주문 생성 실패");
            return;
        }
        uint64_t orderID = db.getLastInsertId();

        // order_items + order_item_options INSERT (스냅샷)
        for (auto& it : req["items"]) {
            std::string menuName = esc(it.value("name", ""));
            int unitPrice = it.value("price", 0);
            int qty       = it.value("qty",   1);
            int menuID    = it.value("menu_id", 0);

            std::string insItem =
                "INSERT INTO order_items (order_id, menu_id, menu_name, price_at_order, quantity) VALUES ("
                + std::to_string(orderID) + ","
                + (menuID > 0 ? std::to_string(menuID) : "NULL") + ",'"
                + menuName + "'," + std::to_string(unitPrice) + "," + std::to_string(qty) + ")";
            db.executeUpdate(insItem);
            uint64_t itemID = db.getLastInsertId();

            // 옵션 스냅샷
            if (it.contains("options")) {
                for (auto& opt : it["options"]) {
                    std::string optName  = esc(opt.value("name",  ""));
                    int         optPrice = opt.value("price", 0);
                    db.executeUpdate(
                        "INSERT INTO order_item_options (order_item_id, option_name, extra_price) VALUES ("
                        + std::to_string(itemID) + ",'" + optName + "'," + std::to_string(optPrice) + ")");
                }
            }
        }

        // payments INSERT
        db.executeUpdate(
            "INSERT INTO payments (order_id, method_type, total_amount, status) VALUES ("
            + std::to_string(orderID) + ",'CARD'," + std::to_string(totalPrice) + ",'SUCCESS')");

        // 포인트 차감 + point_log
        if (usePoint > 0) {
            db.executeUpdate("UPDATE customer_profiles SET point = point - " + std::to_string(usePoint)
                             + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate(
                "INSERT INTO point_log (user_id, amount, reason) VALUES ("
                + std::to_string(uid) + ",-" + std::to_string(usePoint) + ",'주문 사용')");
        }

        // order_status_logs INSERT
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderID) + ",NULL,'PENDING'," + std::to_string(uid) + ")");

        json res; res["status"] = Status::SUCCESS; res["order_id"] = (int)orderID;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_CREATE_ORDER, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleCreateOrder] " << e.what() << std::endl;
        sendErr(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 203: 주문 내역 조회
// ─────────────────────────────────────────────────
void CustomerHandler::handleOrderHistory(Session* session, const std::string&) {
    try {
        auto& db  = MariaDBManager::getInstance();
        int   uid = session->getUserID();
        if (uid == 0) { sendErr(session, CmdCustomer::REQ_ORDER_HISTORY, Status::UNAUTHORIZED, "로그인 필요"); return; }

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name, o.total_price, "
            "p.method_type, o.created_at, o.status "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "LEFT JOIN payments p ON p.order_id = o.order_id "
            "WHERE o.customer_id=" + std::to_string(uid)
            + " ORDER BY o.created_at DESC LIMIT 50");

        json orders = json::array();
        for (auto& r : rows) {
            json o;
            o["order_id"]    = std::stoi(r["order_id"]);
            o["store_name"]  = r["restaurant_name"];
            o["total_price"] = std::stoi(r.count("total_price")  ? r["total_price"]  : "0");
            o["payment_type"]= r.count("method_type")  ? r["method_type"]  : "CARD";
            o["order_time"]  = r.count("created_at")   ? r["created_at"]   : "";
            o["status"]      = statusToInt(r.count("status") ? r["status"] : "PENDING");
            orders.push_back(o);
        }

        json res; res["status"] = Status::SUCCESS; res["orders"] = orders;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_ORDER_HISTORY, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_ORDER_HISTORY, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 204: 주문 상세 조회 (주문현황 화면)
// ─────────────────────────────────────────────────
void CustomerHandler::handleOrderDetail(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int orderID = req.value("order_id", 0);

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name, r.phone AS store_phone, "
            "o.total_price, o.status, o.created_at, o.estimated_minutes, "
            "o.delivery_method "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderID));

        if (rows.empty()) { sendErr(session, CmdCustomer::REQ_ORDER_DETAIL, Status::NOT_FOUND, "주문 없음"); return; }

        auto& r = rows[0];
        json res;
        res["status"]         = Status::SUCCESS;
        res["order_id"]       = orderID;
        res["store_name"]     = r["restaurant_name"];
        res["store_phone"]    = r.count("store_phone")      ? r["store_phone"]      : "";
        res["total_price"]    = std::stoi(r.count("total_price")  ? r["total_price"]  : "0");
        res["order_status"]   = statusToInt(r.count("status") ? r["status"] : "PENDING");
        res["order_time"]     = r.count("created_at")       ? r["created_at"]       : "";
        res["estimated_time"] = r.count("estimated_minutes")? r["estimated_minutes"]: "";

        // 주문 메뉴 목록
        auto itemRows = db.executeQuery(
            "SELECT oi.menu_name, oi.quantity, oi.price_at_order "
            "FROM order_items oi WHERE oi.order_id=" + std::to_string(orderID));
        json items = json::array();
        for (auto& it : itemRows) {
            json item;
            item["name"]     = it["menu_name"];
            item["quantity"] = std::stoi(it.count("quantity")      ? it["quantity"]      : "1");
            item["price"]    = std::stoi(it.count("price_at_order") ? it["price_at_order"]: "0");
            items.push_back(item);
        }
        res["items"] = items;

        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_ORDER_DETAIL, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_ORDER_DETAIL, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 205: 결제 (payments 테이블 업데이트)
// ─────────────────────────────────────────────────
void CustomerHandler::handlePayment(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);
        int orderID = req.value("order_id", 0);

        db.executeUpdate(
            "UPDATE payments SET status='SUCCESS' WHERE order_id=" + std::to_string(orderID));

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_PAYMENT, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_PAYMENT, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 206: 리뷰 작성
// 요청: { order_id, rating:1~5, content }
// ─────────────────────────────────────────────────
void CustomerHandler::handleWriteReview(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   uid = session->getUserID();

        int         orderID = req.value("order_id", 0);
        int         rating  = req.value("rating",   5);
        std::string content = esc(req.value("content", ""));

        if (orderID == 0 || content.empty()) {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "order_id와 content 필수");
            return;
        }

        // 이미 리뷰 작성했는지 확인 (UNIQUE KEY uq_review_order)
        auto chk = db.executeQuery("SELECT review_id FROM reviews WHERE order_id=" + std::to_string(orderID));
        if (!chk.empty()) {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "이미 리뷰를 작성했습니다.");
            return;
        }

        // 주문이 완료(DONE) 상태인지 확인
        auto ordRow = db.executeQuery(
            "SELECT restaurant_id, status FROM orders WHERE order_id=" + std::to_string(orderID)
            + " AND customer_id=" + std::to_string(uid));
        if (ordRow.empty()) {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::NOT_FOUND, "주문을 찾을 수 없습니다.");
            return;
        }
        if (ordRow[0]["status"] != "DONE") {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::FORBIDDEN, "배달 완료 후 리뷰 작성 가능합니다.");
            return;
        }

        db.executeUpdate(
            "INSERT INTO reviews (order_id, customer_id, rating, content) VALUES ("
            + std::to_string(orderID) + "," + std::to_string(uid)
            + "," + std::to_string(rating) + ",'" + content + "')");

        // 음식점 rating_avg 갱신
        int restID = std::stoi(ordRow[0]["restaurant_id"]);
        db.executeUpdate(
            "UPDATE restaurants SET rating_avg = ("
            "SELECT AVG(rv.rating) FROM reviews rv "
            "JOIN orders o ON o.order_id = rv.order_id "
            "WHERE o.restaurant_id=" + std::to_string(restID)
            + ") WHERE restaurant_id=" + std::to_string(restID));

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_WRITE_REVIEW, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 207: 리뷰 목록 조회
// 요청: { store_id }
// ─────────────────────────────────────────────────
void CustomerHandler::handleReviewList(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restID = req.value("store_id", 0);

        auto rows = db.executeQuery(
            "SELECT rv.review_id, u.name AS author, rv.rating, rv.content, "
            "rv.owner_reply, DATE_FORMAT(o.created_at,'%Y-%m-%d') AS created_at "
            "FROM reviews rv "
            "JOIN orders o ON o.order_id = rv.order_id "
            "JOIN users  u ON u.user_id  = rv.customer_id "
            "WHERE o.restaurant_id=" + std::to_string(restID)
            + " ORDER BY o.created_at DESC LIMIT 30");

        json reviews = json::array();
        for (auto& r : rows) {
            json rv;
            rv["id"]          = std::stoi(r["review_id"]);
            rv["author"]      = r["author"];
            rv["rating"]      = std::stoi(r.count("rating")     ? r["rating"]     : "5");
            rv["content"]     = r.count("content")    ? r["content"]    : "";
            rv["owner_reply"] = r.count("owner_reply")? r["owner_reply"]: "";
            rv["created_at"]  = r.count("created_at") ? r["created_at"] : "";
            reviews.push_back(rv);
        }

        json res; res["status"] = Status::SUCCESS; res["reviews"] = reviews;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_REVIEW_LIST, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_REVIEW_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 208: 주문 취소 (PENDING 상태만 가능)
// ─────────────────────────────────────────────────
void CustomerHandler::handleCancelOrder(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   uid     = session->getUserID();
        int   orderID = req.value("order_id", 0);
        std::string reason = esc(req.value("reason", "고객 요청"));

        auto rows = db.executeQuery(
            "SELECT status, total_price FROM orders "
            "WHERE order_id=" + std::to_string(orderID)
            + " AND customer_id=" + std::to_string(uid));

        if (rows.empty()) { sendErr(session, CmdCustomer::REQ_CANCEL_ORDER, Status::NOT_FOUND, "주문 없음"); return; }
        if (rows[0]["status"] != "PENDING") {
            sendErr(session, CmdCustomer::REQ_CANCEL_ORDER, Status::FORBIDDEN, "접수 전 단계에서만 취소 가능합니다.");
            return;
        }

        // 취소 처리
        db.executeUpdate(
            "UPDATE orders SET status='CANCELED', cancel_reason='" + reason
            + "', canceled_at=NOW() WHERE order_id=" + std::to_string(orderID));

        // payments 취소 반영
        db.executeUpdate("UPDATE payments SET status='CANCELED' WHERE order_id=" + std::to_string(orderID));

        // 포인트 환불 (사용한 포인트 복구)
        // point_log에서 해당 주문의 차감 내역 조회 (간략히 total_price 기반)
        // 실제로는 payments.point_used 컬럼으로 관리하는 게 정확함
        // 여기서는 order_id 기준으로 point_log 역산
        auto ptLog = db.executeQuery(
            "SELECT ABS(amount) AS refund FROM point_log "
            "WHERE user_id=" + std::to_string(uid)
            + " AND reason='주문 사용' ORDER BY log_id DESC LIMIT 1");
        if (!ptLog.empty()) {
            int refund = std::stoi(ptLog[0]["refund"]);
            db.executeUpdate("UPDATE customer_profiles SET point = point + " + std::to_string(refund)
                             + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate(
                "INSERT INTO point_log (user_id, amount, reason) VALUES ("
                + std::to_string(uid) + "," + std::to_string(refund) + ",'주문 취소 환불')");
        }

        // order_status_logs
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id, from_status, to_status, reason, changed_by) VALUES ("
            + std::to_string(orderID) + ",'PENDING','CANCELED','" + reason + "'," + std::to_string(uid) + ")");

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_CANCEL_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_CANCEL_ORDER, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// NTF_ORDER_STATUS (210): 서버 → 고객 Push
// ─────────────────────────────────────────────────
void CustomerHandler::pushOrderStatus(Session* session, int orderID, int status, const std::string& msg) {
    json ntf;
    ntf["order_id"] = orderID;
    ntf["status"]   = status;
    ntf["message"]  = msg;
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
}
