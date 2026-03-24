// CustomerHandler.cpp – 실제 DB 스키마(bemin_db_create.sql) 기준 최종 완성본
// 주요 테이블: users, customer_profiles, restaurants, food_categories,
//             menu_categories, menus, option_groups, option_items,
//             orders, order_items, order_item_options, payments,
//             reviews, chat_rooms, chat_messages, customer_coupons, point_log
#include "CustomerHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include "EpollServer.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

// ─────────────────────────────────────────────────
// 내부 헬퍼
// ─────────────────────────────────────────────────
static void sendErr(Session* s, uint16_t proto, uint16_t code, const std::string& msg) {
    json r; r["status"] = code; r["message"] = msg;
    s->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), proto, r.dump());
}

// SQL 인젝션 방지용 단순 이스케이프 (프로덕션에서는 prepared statements 사용)
static std::string esc(const std::string& s) {
    std::string o; o.reserve(s.size() * 2);
    for (char c : s) { if (c=='\'' || c=='\\' || c=='"') o+='\\'; o+=c; }
    return o;
}

// ─────────────────────────────────────────────────
// Dispatcher
// ─────────────────────────────────────────────────
void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    switch (protocol) {
        case CmdCommon::REQ_SIGNUP:         handleSignup      (session, body); break;
        case CmdCommon::REQ_LOGIN:          handleLogin       (session, body); break;
        case CmdCommon::REQ_LOGOUT:         handleLogout      (session, body); break;
        case CmdCommon::REQ_GET_PROFILE:    handleGetProfile  (session, body); break;
        case CmdCommon::REQ_WITHDRAW:       handleWithdraw    (session, body); break;
        case CmdCustomer::REQ_STORE_LIST:   handleStoreList   (session, body); break;
        case CmdCustomer::REQ_MENU_LIST:    handleMenuList    (session, body); break;
        case CmdCustomer::REQ_CREATE_ORDER: handleCreateOrder (session, body); break;
        case CmdCustomer::REQ_ORDER_HISTORY:handleOrderHistory(session, body); break;
        case CmdCustomer::REQ_ORDER_DETAIL: handleOrderDetail (session, body); break;
        case CmdCustomer::REQ_PAYMENT:      handlePayment     (session, body); break;
        case CmdCustomer::REQ_WRITE_REVIEW: handleWriteReview (session, body); break;
        case CmdCustomer::REQ_REVIEW_LIST:  handleReviewList  (session, body); break;
        case CmdCustomer::REQ_CANCEL_ORDER: handleCancelOrder (session, body); break;
        default:
            std::cerr << "[CustomerHandler] 알 수 없는 프로토콜: " << protocol << "\n";
            sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
    }
}

// ─────────────────────────────────────────────────
// 100: 회원가입
// 요청: { id, pw, name, address, phone, role(1=CUSTOMER) }
// ─────────────────────────────────────────────────
void CustomerHandler::handleSignup(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);

        std::string id      = req.value("id",      "");
        std::string pw      = req.value("pw",      "");
        std::string name    = req.value("name",    "");
        std::string address = req.value("address", "");
        std::string phone   = req.value("phone",   "");

        if (id.empty() || pw.empty() || name.empty()) {
            sendErr(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "필수 항목 누락");
            return;
        }

        // 중복 ID 확인
        auto dup = db.executeQuery(
            "SELECT user_id FROM users WHERE login_id='" + esc(id) + "' LIMIT 1");
        if (!dup.empty()) {
            sendErr(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "이미 사용 중인 아이디");
            return;
        }

        // users INSERT
        bool ok = db.executeUpdate(
            "INSERT INTO users (login_id,password,role,name,phone,address,status) VALUES ('"
            + esc(id) + "','" + esc(pw) + "','CUSTOMER','"
            + esc(name) + "','" + esc(phone) + "','" + esc(address) + "','ACTIVE')");
        if (!ok) { sendErr(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "DB 오류"); return; }

        uint64_t uid = db.getLastInsertId();
        // customer_profiles INSERT
        db.executeUpdate(
            "INSERT INTO customer_profiles (user_id, point) VALUES (" + std::to_string(uid) + ", 0)");

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
        json  req = json::parse(body);

        std::string id = req.value("id", "");
        std::string pw = req.value("pw", "");

        // users + customer_profiles JOIN
        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, u.address, u.phone, cp.point "
            "FROM users u "
            "LEFT JOIN customer_profiles cp ON cp.user_id = u.user_id "
            "WHERE u.login_id='" + esc(id) + "' AND u.password='" + esc(pw) + "' "
            "  AND u.role='CUSTOMER' AND u.status='ACTIVE' LIMIT 1");

        if (rows.empty()) {
            sendErr(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "아이디 또는 비밀번호 오류");
            return;
        }

        int uid = std::stoi(rows[0].at("user_id"));
        session->setUserID(uid);
        session->setUserType(static_cast<uint8_t>(ClientType::CUSTOMER));

        std::string token = "ctkn_" + std::to_string(uid);

        json res;
        res["status"]  = Status::SUCCESS;
        res["token"]   = token;
        res["user_id"] = uid;
        res["name"]    = rows[0].at("name");
        res["address"] = rows[0].count("address") ? rows[0].at("address") : "";
        res["point"]   = rows[0].count("point") && !rows[0].at("point").empty()
                         ? std::stoi(rows[0].at("point")) : 0;

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
// 104: 프로필 조회/수정 (주소, 비밀번호)
// ─────────────────────────────────────────────────
void CustomerHandler::handleGetProfile(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        int   uid = session->getUserID();
        if (!uid) { sendErr(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "로그인 필요"); return; }

        json req;
        try { req = body.empty() ? json::object() : json::parse(body); } catch (...) { req = json::object(); }

        // 주소 변경
        if (req.contains("address"))
            db.executeUpdate("UPDATE users SET address='" + esc(req.value("address","")) + "' WHERE user_id=" + std::to_string(uid));

        // 비밀번호 변경
        if (req.contains("new_pw")) {
            std::string old_pw = req.value("old_pw","");
            auto chk = db.executeQuery("SELECT user_id FROM users WHERE user_id=" + std::to_string(uid) + " AND password='" + esc(old_pw) + "'");
            if (chk.empty()) { sendErr(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "현재 비밀번호 불일치"); return; }
            db.executeUpdate("UPDATE users SET password='" + esc(req.value("new_pw","")) + "' WHERE user_id=" + std::to_string(uid));
        }

        auto rows = db.executeQuery(
            "SELECT u.name, u.address, u.phone, cp.point "
            "FROM users u LEFT JOIN customer_profiles cp ON cp.user_id=u.user_id "
            "WHERE u.user_id=" + std::to_string(uid));
        if (rows.empty()) { sendErr(session, CmdCommon::REQ_GET_PROFILE, Status::NOT_FOUND, "사용자 없음"); return; }

        auto& r = rows[0];
        json res;
        res["status"]  = Status::SUCCESS;
        res["name"]    = r.at("name");
        res["address"] = r.count("address") ? r.at("address") : "";
        res["phone"]   = r.at("phone");
        res["point"]   = r.count("point") && !r.at("point").empty() ? std::stoi(r.at("point")) : 0;

        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_GET_PROFILE, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 105: 회원 탈퇴
// ─────────────────────────────────────────────────
void CustomerHandler::handleWithdraw(Session* session, const std::string&) {
    int uid = session->getUserID();
    if (!uid) { sendErr(session, CmdCommon::REQ_WITHDRAW, Status::UNAUTHORIZED, "로그인 필요"); return; }

    MariaDBManager::getInstance().executeUpdate(
        "UPDATE users SET status='DELETED' WHERE user_id=" + std::to_string(uid));
    session->setUserID(0);

    json res; res["status"] = Status::SUCCESS;
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_WITHDRAW, res.dump());
}

// ─────────────────────────────────────────────────
// 200: 매장 목록 조회
// 요청: { category: "치킨"|"전체", ... }
// DB:  restaurants JOIN food_categories
// ─────────────────────────────────────────────────
void CustomerHandler::handleStoreList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        std::string category = req.value("category", "전체");

        std::string q =
            "SELECT r.restaurant_id AS id, r.restaurant_name AS name, "
            "       fc.name AS category, r.base_delivery_fee AS delivery_fee, "
            "       r.min_order_amt, r.rating_avg AS rating, r.address, r.phone, "
            "       r.notice AS description "
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.is_open = TRUE ";
        if (category != "전체")
            q += "AND fc.name='" + esc(category) + "' ";
        q += "ORDER BY r.rating_avg DESC";

        auto rows = db.executeQuery(q);
        json stores = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]           = std::stoi(r.at("id"));
            s["name"]         = r.at("name");
            s["category"]     = r.at("category");
            s["delivery_time"]= "20~40분"; // 실제 추정시간 로직 추후 추가
            s["distance"]     = 0.0;       // GPS 기반 계산 추후 추가
            s["min_order"]    = std::stoi(r.count("min_order_amt") ? r.at("min_order_amt") : "0");
            s["delivery_fee"] = std::stoi(r.count("delivery_fee")  ? r.at("delivery_fee")  : "0");
            s["rating"]       = r.count("rating") && !r.at("rating").empty()
                                ? std::stod(r.at("rating")) : 0.0;
            s["address"]      = r.count("address") ? r.at("address") : "";
            s["phone"]        = r.count("phone")   ? r.at("phone")   : "";
            s["description"]  = r.count("description") ? r.at("description") : "";
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
// DB:  menus ← menu_categories ← restaurants
//      + option_groups + option_items
// ─────────────────────────────────────────────────
void CustomerHandler::handleMenuList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);

        int         storeID = req.value("store_id",     0);
        std::string subCat  = req.value("sub_category", "전체");

        if (!storeID) { sendErr(session, CmdCustomer::REQ_MENU_LIST, Status::BAD_REQUEST, "store_id 필요"); return; }

        // 메뉴 카테고리 목록
        std::string catQ =
            "SELECT mc.menu_category_id, mc.category_name "
            "FROM menu_categories mc "
            "WHERE mc.restaurant_id=" + std::to_string(storeID) + " ORDER BY mc.menu_category_id";
        auto catRows = db.executeQuery(catQ);

        // 메뉴 조회
        std::string menuQ =
            "SELECT m.menu_id, m.menu_name, m.description, m.price, m.is_sold_out, "
            "       mc.category_name AS sub_category "
            "FROM menus m "
            "JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
            "WHERE mc.restaurant_id=" + std::to_string(storeID);
        if (subCat != "전체")
            menuQ += " AND mc.category_name='" + esc(subCat) + "'";
        menuQ += " ORDER BY mc.menu_category_id, m.menu_id";

        auto menuRows = db.executeQuery(menuQ);
        json menus = json::array();

        for (auto& mr : menuRows) {
            int menuID = std::stoi(mr.at("menu_id"));
            json m;
            m["id"]           = menuID;
            m["name"]         = mr.at("menu_name");
            m["desc"]         = mr.count("description") ? mr.at("description") : "";
            m["price"]        = std::stoi(mr.at("price"));
            m["is_sold_out"]  = (mr.at("is_sold_out") == "1");
            m["sub_category"] = mr.at("sub_category");

            // option_groups + option_items 조회
            auto ogRows = db.executeQuery(
                "SELECT og.option_group_id, og.group_name, og.is_required, og.max_select "
                "FROM option_groups og WHERE og.menu_id=" + std::to_string(menuID));

            json optGroups = json::array();
            for (auto& og : ogRows) {
                int ogID = std::stoi(og.at("option_group_id"));
                json grp;
                grp["group_id"]   = ogID;
                grp["group_name"] = og.at("group_name");
                grp["is_required"]= (og.at("is_required") == "1");
                grp["max_select"] = std::stoi(og.at("max_select"));

                auto oiRows = db.executeQuery(
                    "SELECT option_item_id, item_name, extra_price "
                    "FROM option_items WHERE option_group_id=" + std::to_string(ogID));
                json items = json::array();
                for (auto& oi : oiRows) {
                    json opt;
                    opt["option_id"]  = std::stoi(oi.at("option_item_id"));
                    opt["name"]       = oi.at("item_name");
                    opt["price"]      = std::stoi(oi.at("extra_price"));
                    items.push_back(opt);
                }
                grp["options"] = items;
                optGroups.push_back(grp);
            }
            m["option_groups"] = optGroups;
            menus.push_back(m);
        }

        // 서브카테고리 목록도 함께 반환 (ScrollMenu 탭용)
        json subCats = json::array();
        subCats.push_back("전체");
        for (auto& cr : catRows)
            subCats.push_back(cr.at("category_name"));

        json res;
        res["status"]      = Status::SUCCESS;
        res["menus"]       = menus;
        res["sub_categories"] = subCats;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_MENU_LIST, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_MENU_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 202: 주문 생성
// 요청: { store_id, items:[{menu_id,qty,price,options:[{option_item_id,extra_price}]}],
//         is_delivery, delivery_address, payment_method_id, use_point, coupon_id }
// DB:  orders → order_items → order_item_options → payments → point_log
// ─────────────────────────────────────────────────
void CustomerHandler::handleCreateOrder(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);

        int   uid     = session->getUserID();
        int   storeID = req.value("store_id",   0);
        bool  isDel   = req.value("is_delivery", true);
        int   usePoint= req.value("use_point",  0);
        std::string delAddr = req.value("delivery_address", "");

        if (!uid || !storeID) {
            sendErr(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "필수 파라미터 누락");
            return;
        }

        // 총 금액 계산
        int totalPrice = 0;
        for (auto& it : req["items"]) {
            int qty   = it.value("qty", 1);
            int price = it.value("price", 0);
            totalPrice += price * qty;
            // 옵션 금액 합산
            if (it.contains("options"))
                for (auto& opt : it["options"])
                    totalPrice += opt.value("extra_price", 0) * qty;
        }

        // 포인트 차감 (customer_profiles에서 보유 확인)
        if (usePoint > 0) {
            auto pr = db.executeQuery(
                "SELECT point FROM customer_profiles WHERE user_id=" + std::to_string(uid));
            int held = (!pr.empty() && !pr[0].at("point").empty()) ? std::stoi(pr[0].at("point")) : 0;
            if (usePoint > held) {
                sendErr(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "포인트 부족");
                return;
            }
            totalPrice -= usePoint;
        }

        std::string method = isDel ? "DELIVERY" : "PICKUP";

        // orders INSERT
        bool ok = db.executeUpdate(
            "INSERT INTO orders (customer_id, restaurant_id, status, total_price, "
            "delivery_method, delivery_address) VALUES ("
            + std::to_string(uid) + "," + std::to_string(storeID) + ","
            "'PENDING'," + std::to_string(totalPrice) + ",'"
            + method + "','" + esc(delAddr) + "')");
        if (!ok) { sendErr(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, "주문 생성 실패"); return; }
        uint64_t orderID = db.getLastInsertId();

        // order_items + order_item_options INSERT
        for (auto& it : req["items"]) {
            int menuID = it.value("menu_id", 0);
            int qty    = it.value("qty",     1);
            int price  = it.value("price",   0);

            db.executeUpdate(
                "INSERT INTO order_items (order_id, menu_id, quantity, unit_price) VALUES ("
                + std::to_string(orderID) + "," + std::to_string(menuID) + ","
                + std::to_string(qty) + "," + std::to_string(price) + ")");
            uint64_t oiID = db.getLastInsertId();

            if (it.contains("options")) {
                for (auto& opt : it["options"]) {
                    int optItemID  = opt.value("option_item_id", 0);
                    int extraPrice = opt.value("extra_price",    0);
                    if (optItemID)
                        db.executeUpdate(
                            "INSERT INTO order_item_options (order_item_id, option_item_id, extra_price) VALUES ("
                            + std::to_string(oiID) + "," + std::to_string(optItemID) + ","
                            + std::to_string(extraPrice) + ")");
                }
            }
        }

        // 포인트 차감 처리
        if (usePoint > 0) {
            db.executeUpdate(
                "UPDATE customer_profiles SET point = point - " + std::to_string(usePoint)
                + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate(
                "INSERT INTO point_log (user_id, change_amount, reason, order_id) VALUES ("
                + std::to_string(uid) + ",-" + std::to_string(usePoint) + ",'ORDER_USE',"
                + std::to_string(orderID) + ")");
        }

        // order_status_logs INSERT
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderID) + ",'','PENDING'," + std::to_string(uid) + ")");

        json res;
        res["status"]   = Status::SUCCESS;
        res["order_id"] = (int)orderID;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_CREATE_ORDER, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleCreateOrder] " << e.what() << "\n";
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
        if (!uid) { sendErr(session, CmdCustomer::REQ_ORDER_HISTORY, Status::UNAUTHORIZED, "로그인 필요"); return; }

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       o.total_price, o.status, o.created_at AS order_time "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.customer_id=" + std::to_string(uid)
            + " ORDER BY o.created_at DESC LIMIT 50");

        // status 문자열 → 숫자 매핑
        auto statusToInt = [](const std::string& s) {
            if (s == "CANCELED" || s == "REJECTED") return 0;
            if (s == "PENDING")         return 1;
            if (s == "ACCEPTED" || s == "COOKING") return 2;
            if (s == "WAITING_PICKUP" || s == "DELIVERING") return 3;
            if (s == "DONE")            return 4;
            return 0;
        };

        json orders = json::array();
        for (auto& r : rows) {
            json o;
            o["order_id"]    = std::stoi(r.at("order_id"));
            o["store_name"]  = r.at("store_name");
            o["total_price"] = std::stoi(r.count("total_price") ? r.at("total_price") : "0");
            o["status"]      = statusToInt(r.count("status") ? r.at("status") : "");
            o["order_time"]  = r.count("order_time") ? r.at("order_time") : "";
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
        auto& db    = MariaDBManager::getInstance();
        json  req   = json::parse(body);
        int   orderID = req.value("order_id", 0);

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, r.phone AS store_phone, "
            "       o.total_price, o.status, o.created_at AS order_time, "
            "       o.delivery_method, o.delivery_address "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderID));

        if (rows.empty()) { sendErr(session, CmdCustomer::REQ_ORDER_DETAIL, Status::NOT_FOUND, "주문 없음"); return; }

        auto& r = rows[0];
        json res;
        res["status"]       = Status::SUCCESS;
        res["order_id"]     = orderID;
        res["store_name"]   = r.at("store_name");
        res["store_phone"]  = r.count("store_phone")  ? r.at("store_phone")  : "";
        res["total_price"]  = std::stoi(r.count("total_price") ? r.at("total_price") : "0");
        res["order_status"] = r.count("status") ? r.at("status") : "";
        res["order_time"]   = r.count("order_time") ? r.at("order_time") : "";

        // 주문 메뉴 항목
        auto itemRows = db.executeQuery(
            "SELECT m.menu_name AS name, oi.quantity, oi.unit_price AS price "
            "FROM order_items oi JOIN menus m ON oi.menu_id = m.menu_id "
            "WHERE oi.order_id=" + std::to_string(orderID));
        json items = json::array();
        for (auto& it : itemRows) {
            json item;
            item["name"]     = it.at("name");
            item["quantity"] = std::stoi(it.count("quantity") ? it.at("quantity") : "1");
            item["price"]    = std::stoi(it.count("price")    ? it.at("price")    : "0");
            items.push_back(item);
        }
        res["items"] = items;

        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_ORDER_DETAIL, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_ORDER_DETAIL, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 205: 결제 처리 (payments 테이블)
// 요청: { order_id, payment_method_id, amount }
// ─────────────────────────────────────────────────
void CustomerHandler::handlePayment(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int orderID  = req.value("order_id", 0);
        int pmID     = req.value("payment_method_id", 0);
        int amount   = req.value("amount", 0);

        bool ok = db.executeUpdate(
            "INSERT INTO payments (order_id, payment_method_id, amount, paid_at) VALUES ("
            + std::to_string(orderID) + "," + std::to_string(pmID) + ","
            + std::to_string(amount) + ",NOW())");

        if (ok) db.executeUpdate(
            "UPDATE orders SET status='PENDING' WHERE order_id=" + std::to_string(orderID));

        json res; res["status"] = ok ? Status::SUCCESS : Status::SERVER_ERROR;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_PAYMENT, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_PAYMENT, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// 206: 리뷰 작성 (reviews 테이블)
// reviews: review_id, order_id, customer_id, rating, content, image_url
// ─────────────────────────────────────────────────
void CustomerHandler::handleWriteReview(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);

        int         uid     = session->getUserID();
        int         orderID = req.value("order_id", 0);
        int         rating  = req.value("rating",   5);
        std::string content = req.value("content",  "");

        if (!uid || !orderID || content.empty()) {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "필수 항목 누락");
            return;
        }

        // 주문 소유 확인 + 완료 상태 확인
        auto chk = db.executeQuery(
            "SELECT order_id FROM orders WHERE order_id=" + std::to_string(orderID)
            + " AND customer_id=" + std::to_string(uid) + " AND status='DONE'");
        if (chk.empty()) {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::FORBIDDEN, "배달 완료된 주문만 리뷰 가능");
            return;
        }

        // 중복 리뷰 방지 (order_id UNIQUE)
        auto dup = db.executeQuery(
            "SELECT review_id FROM reviews WHERE order_id=" + std::to_string(orderID));
        if (!dup.empty()) {
            sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "이미 리뷰를 작성했습니다");
            return;
        }

        bool ok = db.executeUpdate(
            "INSERT INTO reviews (order_id, customer_id, rating, content) VALUES ("
            + std::to_string(orderID) + "," + std::to_string(uid) + ","
            + std::to_string(rating) + ",'" + esc(content) + "')");

        if (!ok) { sendErr(session, CmdCustomer::REQ_WRITE_REVIEW, Status::SERVER_ERROR, "저장 실패"); return; }

        // restaurant 평점 갱신 (rating_avg)
        db.executeUpdate(
            "UPDATE restaurants r SET r.rating_avg = ("
            "SELECT AVG(rv.rating) FROM reviews rv "
            "JOIN orders o ON o.order_id=rv.order_id "
            "WHERE o.restaurant_id=r.restaurant_id) "
            "WHERE r.restaurant_id=("
            "SELECT restaurant_id FROM orders WHERE order_id=" + std::to_string(orderID) + ")");

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
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   storeID = req.value("store_id", 0);

        auto rows = db.executeQuery(
            "SELECT rv.review_id, u.name AS author, rv.rating, rv.content, "
            "       rv.owner_reply, rv.replied_at "
            "FROM reviews rv "
            "JOIN orders o  ON o.order_id    = rv.order_id "
            "JOIN users u   ON u.user_id     = rv.customer_id "
            "WHERE o.restaurant_id=" + std::to_string(storeID)
            + " ORDER BY rv.review_id DESC LIMIT 30");

        json reviews = json::array();
        for (auto& r : rows) {
            json rv;
            rv["id"]          = std::stoi(r.at("review_id"));
            rv["author"]      = r.at("author");
            rv["rating"]      = std::stoi(r.count("rating")  ? r.at("rating")  : "5");
            rv["content"]     = r.count("content")      ? r.at("content")      : "";
            rv["owner_reply"] = r.count("owner_reply")  ? r.at("owner_reply")  : "";
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
        int   uid    = session->getUserID();
        int   orderID= req.value("order_id", 0);

        auto rows = db.executeQuery(
            "SELECT status FROM orders WHERE order_id=" + std::to_string(orderID)
            + " AND customer_id=" + std::to_string(uid));
        if (rows.empty()) { sendErr(session, CmdCustomer::REQ_CANCEL_ORDER, Status::NOT_FOUND, "주문 없음"); return; }

        std::string st = rows[0].at("status");
        if (st != "PENDING") {
            sendErr(session, CmdCustomer::REQ_CANCEL_ORDER, Status::FORBIDDEN,
                    "주문접수 상태에서만 취소 가능합니다");
            return;
        }

        db.executeUpdate(
            "UPDATE orders SET status='CANCELED' WHERE order_id=" + std::to_string(orderID));

        // 포인트 복구 (point_log 확인)
        auto plRows = db.executeQuery(
            "SELECT change_amount FROM point_log WHERE order_id=" + std::to_string(orderID)
            + " AND reason='ORDER_USE'");
        if (!plRows.empty()) {
            int usedPoint = std::abs(std::stoi(plRows[0].at("change_amount")));
            db.executeUpdate(
                "UPDATE customer_profiles SET point = point + " + std::to_string(usedPoint)
                + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate(
                "INSERT INTO point_log (user_id, change_amount, reason, order_id) VALUES ("
                + std::to_string(uid) + "," + std::to_string(usedPoint) + ",'CANCEL_REFUND',"
                + std::to_string(orderID) + ")");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_CANCEL_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendErr(session, CmdCustomer::REQ_CANCEL_ORDER, Status::SERVER_ERROR, e.what());
    }
}

// ─────────────────────────────────────────────────
// NTF_ORDER_STATUS (210): 서버 → 고객 Push
// ─────────────────────────────────────────────────
void CustomerHandler::pushOrderStatus(Session* session, int orderID,
                                       int status, const std::string& msg) {
    json ntf;
    ntf["order_id"] = orderID;
    ntf["status"]   = status;
    ntf["message"]  = msg;
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                        CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
}
