// CustomerHandler.cpp – bemin_db 실제 스키마 기준 완성본
// 주요 테이블: users, customer_profiles, restaurants, food_categories,
//             menu_categories, menus, option_groups, option_items,
//             orders, order_items, order_item_options, payments,
//             reviews, customer_coupons, point_log, chat_rooms
#include "CustomerHandler.h"
#include "Session.h"
#include "MariaDBManager.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ── 공통 헬퍼 ──────────────────────────────────────
static void sendError(Session* s, uint16_t proto, uint16_t code, const std::string& msg) {
    json r; r["status"] = code; r["message"] = msg;
    s->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), proto, r.dump());
}

static std::string esc(const std::string& s) {
    std::string o; o.reserve(s.size() * 2);
    for (char c : s) { if (c=='\'' || c=='\\' || c=='"') o+='\\'; o+=c; }
    return o;
}

// ── Dispatcher ─────────────────────────────────────
void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    switch (protocol) {
        case CmdCommon::REQ_SIGNUP:         handleSignup      (session, jsonBody); break;
        case CmdCommon::REQ_LOGIN:          handleLogin       (session, jsonBody); break;
        case CmdCommon::REQ_LOGOUT:         handleLogout      (session, jsonBody); break;
        case CmdCommon::REQ_GET_PROFILE:    handleGetProfile  (session, jsonBody); break;
        case CmdCustomer::REQ_STORE_LIST:   handleStoreList   (session, jsonBody); break;
        case CmdCustomer::REQ_MENU_LIST:    handleMenuList    (session, jsonBody); break;
        case CmdCustomer::REQ_CREATE_ORDER: handleCreateOrder (session, jsonBody); break;
        case CmdCustomer::REQ_ORDER_HISTORY:handleOrderHistory(session, jsonBody); break;
        case CmdCustomer::REQ_ORDER_DETAIL: handleOrderDetail (session, jsonBody); break;
        case CmdCustomer::REQ_PAYMENT:      handlePayment     (session, jsonBody); break;
        case CmdCustomer::REQ_WRITE_REVIEW: handleWriteReview (session, jsonBody); break;
        case CmdCustomer::REQ_REVIEW_LIST:  handleReviewList  (session, jsonBody); break;
        case CmdCustomer::REQ_CANCEL_ORDER: handleCancelOrder (session, jsonBody); break;
        default:
            sendError(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
    }
}

// ──────────────────────────────────────────────────────
// 100: 회원가입
// 요청: { id, pw, name, phone, address, role:1 }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleSignup(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);

        std::string loginId = esc(req.value("id",      ""));
        std::string pw      = esc(req.value("pw",      ""));
        std::string name    = esc(req.value("name",    ""));
        std::string phone   = esc(req.value("phone",   ""));
        std::string address = esc(req.value("address", ""));

        if (loginId.empty() || pw.empty() || name.empty()) {
            sendError(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "필수 항목 누락");
            return;
        }

        // 중복 ID 확인
        auto dup = db.executeQuery(
            "SELECT user_id FROM users WHERE login_id='" + loginId + "' LIMIT 1");
        if (!dup.empty()) {
            sendError(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "이미 사용 중인 아이디");
            return;
        }

        // users INSERT
        if (!db.executeUpdate(
            "INSERT INTO users (login_id,password,role,name,phone,address,status) "
            "VALUES ('" + loginId + "','" + pw + "','CUSTOMER','"
            + name + "','" + phone + "','" + address + "','ACTIVE')")) {
            sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "DB 오류");
            return;
        }
        uint64_t uid = db.getLastInsertId();

        // customer_profiles INSERT (포인트 0으로 초기화)
        db.executeUpdate(
            "INSERT INTO customer_profiles (user_id, point) VALUES ("
            + std::to_string(uid) + ", 0)");

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_SIGNUP, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 101: 로그인
// 응답: { status, token, user_id, name, address, point }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleLogin(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body);

        std::string loginId = esc(req.value("id", ""));
        std::string pw      = esc(req.value("pw", ""));

        auto rows = db.executeQuery(
            "SELECT u.user_id, u.name, u.address, u.phone, cp.point "
            "FROM users u "
            "LEFT JOIN customer_profiles cp ON cp.user_id = u.user_id "
            "WHERE u.login_id='" + loginId + "' AND u.password='" + pw
            + "' AND u.role='CUSTOMER' AND u.status='ACTIVE' LIMIT 1");

        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED, "아이디 또는 비밀번호가 틀렸습니다");
            return;
        }

        int uid = std::stoi(rows[0]["user_id"]);
        session->setUserID(uid);
        session->setUserType(static_cast<uint8_t>(ClientType::CUSTOMER));

        // request_logs 기록
        db.executeUpdate(
            "INSERT INTO request_logs (request_id, user_id, action) "
            "VALUES (UUID()," + std::to_string(uid) + ",'LOGIN')");

        json res;
        res["status"]  = Status::SUCCESS;
        res["token"]   = "tkn_" + loginId + "_" + std::to_string(uid);
        res["user_id"] = uid;
        res["name"]    = rows[0]["name"];
        res["address"] = rows[0].count("address") ? rows[0]["address"] : "";
        res["point"]   = rows[0].count("point") && !rows[0]["point"].empty()
                         ? std::stoi(rows[0]["point"]) : 0;

        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_LOGIN, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 102: 로그아웃
// ──────────────────────────────────────────────────────
void CustomerHandler::handleLogout(Session* session, const std::string& body) {
    session->setUserID(0);
    json res; res["status"] = Status::SUCCESS;
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_LOGOUT, res.dump());
}

// ──────────────────────────────────────────────────────
// 104: 프로필 조회/수정
// ──────────────────────────────────────────────────────
void CustomerHandler::handleGetProfile(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   uid = session->getUserID();
        if (uid == 0) { sendError(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "로그인 필요"); return; }

        // 주소 변경
        if (req.contains("address")) {
            db.executeUpdate("UPDATE users SET address='" + esc(req["address"].get<std::string>())
                             + "' WHERE user_id=" + std::to_string(uid));
        }

        auto rows = db.executeQuery(
            "SELECT u.name, u.address, u.phone, cp.point "
            "FROM users u LEFT JOIN customer_profiles cp ON cp.user_id=u.user_id "
            "WHERE u.user_id=" + std::to_string(uid));

        if (rows.empty()) { sendError(session, CmdCommon::REQ_GET_PROFILE, Status::NOT_FOUND, "사용자 없음"); return; }

        auto& r = rows[0];
        json res;
        res["status"]  = Status::SUCCESS;
        res["name"]    = r["name"];
        res["address"] = r.count("address") ? r["address"] : "";
        res["phone"]   = r["phone"];
        res["point"]   = r.count("point") && !r["point"].empty() ? std::stoi(r["point"]) : 0;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCommon::REQ_GET_PROFILE, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 200: 매장 목록 조회
// 요청: { token, category }  (category: food_categories.category_name)
// 응답: { stores:[{id,name,category,delivery_time,distance,min_order,delivery_fee,rating,...}] }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleStoreList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        std::string category = req.value("category", "전체");

        std::string q =
            "SELECT r.restaurant_id AS id, r.restaurant_name AS name, "
            "       fc.category_name AS category, r.address, r.phone, "
            "       r.min_order_amt AS min_order, r.base_delivery_fee AS delivery_fee, "
            "       r.rating_avg AS rating, r.notice AS description, "
            "       '20~30분' AS delivery_time, 0.0 AS distance "
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.is_open = 1 ";
        if (category != "전체")
            q += "AND fc.category_name='" + esc(category) + "' ";
        q += "ORDER BY r.rating_avg DESC";

        auto rows = db.executeQuery(q);
        json stores = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]           = std::stoi(r["id"]);
            s["name"]         = r["name"];
            s["category"]     = r["category"];
            s["address"]      = r.count("address")  ? r["address"]  : "";
            s["phone"]        = r.count("phone")    ? r["phone"]    : "";
            s["min_order"]    = r.count("min_order")    && !r["min_order"].empty()    ? std::stoi(r["min_order"])    : 0;
            s["delivery_fee"] = r.count("delivery_fee") && !r["delivery_fee"].empty() ? std::stoi(r["delivery_fee"]) : 0;
            s["rating"]       = r.count("rating")   && !r["rating"].empty()   ? std::stod(r["rating"])   : 0.0;
            s["delivery_time"]= r["delivery_time"];
            s["distance"]     = std::stod(r["distance"]);
            s["description"]  = r.count("description") ? r["description"] : "";
            stores.push_back(s);
        }

        json res; res["status"] = Status::SUCCESS; res["stores"] = stores;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_STORE_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_STORE_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 201: 메뉴 목록 조회
// 요청: { token, store_id: restaurant_id, sub_category }
// 응답: { menus:[{id,name,desc,price,sub_category,options:[{name,price}]}] }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleMenuList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restaurantId = req.value("store_id",    0);
        std::string subCat = req.value("sub_category","전체");

        if (restaurantId == 0) {
            sendError(session, CmdCustomer::REQ_MENU_LIST, Status::BAD_REQUEST, "store_id 필요"); return;
        }

        // 메뉴 카테고리 → 메뉴 JOIN
        std::string q =
            "SELECT m.menu_id AS id, m.menu_name AS name, m.description AS desc, "
            "       m.price, mc.category_name AS sub_category "
            "FROM menus m "
            "JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
            "WHERE mc.restaurant_id=" + std::to_string(restaurantId)
            + " AND m.is_sold_out=0 ";
        if (subCat != "전체")
            q += "AND mc.category_name='" + esc(subCat) + "' ";
        q += "ORDER BY mc.sort_order, m.menu_id";

        auto rows = db.executeQuery(q);
        json menus = json::array();

        for (auto& r : rows) {
            int menuId = std::stoi(r["id"]);
            json m;
            m["id"]           = menuId;
            m["name"]         = r["name"];
            m["desc"]         = r.count("desc")         ? r["desc"]         : "";
            m["price"]        = r.count("price")        && !r["price"].empty()        ? std::stoi(r["price"])        : 0;
            m["sub_category"] = r.count("sub_category") ? r["sub_category"] : "";

            // 옵션 그룹 → 옵션 아이템 조회
            auto ogRows = db.executeQuery(
                "SELECT og.option_group_id, og.group_name, og.is_essential "
                "FROM option_groups og WHERE og.menu_id=" + std::to_string(menuId));

            json opts = json::array();
            for (auto& og : ogRows) {
                int ogId = std::stoi(og["option_group_id"]);
                auto oiRows = db.executeQuery(
                    "SELECT option_name AS name, extra_price AS price "
                    "FROM option_items WHERE option_group_id=" + std::to_string(ogId));
                for (auto& oi : oiRows) {
                    json o;
                    o["name"]  = oi["name"];
                    o["price"] = oi.count("price") && !oi["price"].empty() ? std::stoi(oi["price"]) : 0;
                    opts.push_back(o);
                }
            }
            m["options"] = opts;
            menus.push_back(m);
        }

        json res; res["status"] = Status::SUCCESS; res["menus"] = menus;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_MENU_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_MENU_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 202: 주문 생성
// 요청: { token, store_id, items:[{menu_id,menu_name,qty,price,
//         options:[{option_item_id,option_name,extra_price}]}],
//         is_delivery, delivery_address, card_id, use_point, coupon_id }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleCreateOrder(Session* session, const std::string& body) {
    try {
        auto& db   = MariaDBManager::getInstance();
        json  req  = json::parse(body);
        int   uid  = session->getUserID();
        int   restId = req.value("store_id", 0);

        if (uid == 0 || restId == 0) {
            sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "파라미터 오류"); return;
        }

        bool        isDelivery = req.value("is_delivery", true);
        std::string delivAddr  = esc(req.value("delivery_address", ""));
        int         usePoint   = req.value("use_point", 0);
        std::string delivMethod= isDelivery ? "DELIVERY" : "PICKUP";

        // 총액 계산
        int totalPrice = 0;
        for (auto& item : req["items"]) {
            int qty   = item.value("qty",   1);
            int price = item.value("price", 0);
            totalPrice += price * qty;
            // 옵션 추가가
            if (item.contains("options")) {
                for (auto& opt : item["options"])
                    totalPrice += opt.value("extra_price", 0) * qty;
            }
        }
        if (usePoint > 0) totalPrice -= usePoint;

        // orders INSERT
        if (!db.executeUpdate(
            "INSERT INTO orders (customer_id,restaurant_id,status,total_price,"
            "delivery_method,delivery_address) VALUES ("
            + std::to_string(uid) + "," + std::to_string(restId)
            + ",'PENDING'," + std::to_string(totalPrice)
            + ",'" + delivMethod + "','" + delivAddr + "')")) {
            sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, "주문 생성 실패"); return;
        }
        uint64_t orderId = db.getLastInsertId();

        // order_items + order_item_options INSERT (스냅샷)
        for (auto& item : req["items"]) {
            int menuId  = item.value("menu_id",   0);
            int qty     = item.value("qty",       1);
            int price   = item.value("price",     0);
            std::string menuName = esc(item.value("menu_name", ""));

            db.executeUpdate(
                "INSERT INTO order_items (order_id,menu_id,menu_name,price_at_order,quantity) VALUES ("
                + std::to_string(orderId) + "," + std::to_string(menuId)
                + ",'" + menuName + "'," + std::to_string(price)
                + "," + std::to_string(qty) + ")");
            uint64_t oiId = db.getLastInsertId();

            if (item.contains("options")) {
                for (auto& opt : item["options"]) {
                    int    optItemId  = opt.value("option_item_id", 0);
                    std::string oName = esc(opt.value("option_name",  ""));
                    int    extraPrice = opt.value("extra_price", 0);
                    db.executeUpdate(
                        "INSERT INTO order_item_options "
                        "(order_item_id,option_item_id,option_name,extra_price) VALUES ("
                        + std::to_string(oiId) + "," + std::to_string(optItemId)
                        + ",'" + oName + "'," + std::to_string(extraPrice) + ")");
                }
            }
        }

        // 포인트 차감
        if (usePoint > 0) {
            db.executeUpdate(
                "UPDATE customer_profiles SET point=point-" + std::to_string(usePoint)
                + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate(
                "INSERT INTO point_log (user_id,amount,reason) VALUES ("
                + std::to_string(uid) + ",-" + std::to_string(usePoint)
                + ",'주문 포인트 사용')");
        }

        // order_status_logs
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id,from_status,to_status,changed_by) "
            "VALUES (" + std::to_string(orderId) + ",NULL,'PENDING'," + std::to_string(uid) + ")");

        json res; res["status"] = Status::SUCCESS; res["order_id"] = (int)orderId;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_CREATE_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 203: 주문 내역 조회
// ──────────────────────────────────────────────────────
void CustomerHandler::handleOrderHistory(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        int   uid = session->getUserID();
        if (uid == 0) { sendError(session, CmdCustomer::REQ_ORDER_HISTORY, Status::UNAUTHORIZED, "로그인 필요"); return; }

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       o.total_price, o.status, o.created_at AS order_time, "
            "       p.method_type AS payment_type "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "LEFT JOIN payments p ON p.order_id = o.order_id "
            "WHERE o.customer_id=" + std::to_string(uid)
            + " ORDER BY o.created_at DESC LIMIT 50");

        json orders = json::array();
        for (auto& r : rows) {
            json o;
            o["order_id"]    = std::stoi(r["order_id"]);
            o["store_name"]  = r["store_name"];
            o["total_price"] = r.count("total_price") && !r["total_price"].empty() ? std::stoi(r["total_price"]) : 0;
            o["status"]      = r["status"];
            o["order_time"]  = r.count("order_time") ? r["order_time"] : "";
            o["payment_type"]= r.count("payment_type") ? r["payment_type"] : "CARD";
            orders.push_back(o);
        }

        json res; res["status"] = Status::SUCCESS; res["orders"] = orders;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_ORDER_HISTORY, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_ORDER_HISTORY, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 204: 주문 상세 (주문현황 화면)
// ──────────────────────────────────────────────────────
void CustomerHandler::handleOrderDetail(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   orderId = req.value("order_id", 0);

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, r.phone AS store_phone, "
            "       o.total_price, o.status, o.created_at AS order_time, "
            "       o.estimated_minutes, o.delivery_address "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderId));

        if (rows.empty()) { sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::NOT_FOUND, "주문 없음"); return; }

        auto& r = rows[0];
        json res;
        res["status"]         = Status::SUCCESS;
        res["order_id"]       = orderId;
        res["store_name"]     = r["store_name"];
        res["store_phone"]    = r.count("store_phone")  ? r["store_phone"]  : "";
        res["total_price"]    = r.count("total_price")  && !r["total_price"].empty()  ? std::stoi(r["total_price"]) : 0;
        res["order_status"]   = r["status"];
        res["order_time"]     = r.count("order_time")   ? r["order_time"]   : "";
        res["estimated_time"] = r.count("estimated_minutes") && !r["estimated_minutes"].empty()
                                ? r["estimated_minutes"] : "0";

        // 주문 메뉴 목록
        auto itemRows = db.executeQuery(
            "SELECT menu_name AS name, quantity, price_at_order AS price "
            "FROM order_items WHERE order_id=" + std::to_string(orderId));
        json items = json::array();
        for (auto& it : itemRows) {
            json item;
            item["name"]     = it["name"];
            item["quantity"] = it.count("quantity") && !it["quantity"].empty() ? std::stoi(it["quantity"]) : 1;
            item["price"]    = it.count("price")    && !it["price"].empty()    ? std::stoi(it["price"])    : 0;
            items.push_back(item);
        }
        res["items"] = items;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_ORDER_DETAIL, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 205: 결제 확정 (payments 테이블 INSERT)
// ──────────────────────────────────────────────────────
void CustomerHandler::handlePayment(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   orderId     = req.value("order_id",   0);
        std::string method= req.value("method_type","CARD");
        int   amount      = req.value("amount",     0);

        // payments 테이블에 결제 결과 기록
        db.executeUpdate(
            "INSERT INTO payments (order_id,method_type,total_amount,status) VALUES ("
            + std::to_string(orderId) + ",'" + esc(method) + "',"
            + std::to_string(amount) + ",'SUCCESS') "
            "ON DUPLICATE KEY UPDATE status='SUCCESS',method_type='" + esc(method) + "'");

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_PAYMENT, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_PAYMENT, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 206: 리뷰 작성
// 요청: { token, order_id, rating:1~5, content }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleWriteReview(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   uid     = session->getUserID();
        int   orderId = req.value("order_id", 0);
        int   rating  = req.value("rating",   5);
        std::string content = esc(req.value("content", ""));

        if (uid == 0 || orderId == 0 || content.empty()) {
            sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "파라미터 오류"); return;
        }

        // 해당 주문이 DONE 상태인지 + 본인 주문인지 확인
        auto chk = db.executeQuery(
            "SELECT order_id FROM orders "
            "WHERE order_id=" + std::to_string(orderId)
            + " AND customer_id=" + std::to_string(uid)
            + " AND status='DONE'");
        if (chk.empty()) {
            sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::FORBIDDEN, "배달 완료된 주문에만 리뷰 가능"); return;
        }

        // 중복 리뷰 방지 (UNIQUE KEY uq_review_order)
        if (!db.executeUpdate(
            "INSERT IGNORE INTO reviews (order_id,customer_id,rating,content) VALUES ("
            + std::to_string(orderId) + "," + std::to_string(uid)
            + "," + std::to_string(rating) + ",'" + content + "')")) {
            sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "이미 리뷰를 작성했습니다"); return;
        }

        // 매장 평균 별점 갱신
        db.executeUpdate(
            "UPDATE restaurants r "
            "JOIN orders o ON o.restaurant_id = r.restaurant_id "
            "SET r.rating_avg = ("
            "  SELECT AVG(rv.rating) FROM reviews rv "
            "  JOIN orders ov ON ov.order_id = rv.order_id "
            "  WHERE ov.restaurant_id = r.restaurant_id"
            ") WHERE o.order_id=" + std::to_string(orderId));

        // 리뷰 작성 포인트 적립 (정책: system_policies에서 읽어옴)
        auto policy = db.executeQuery(
            "SELECT policy_value FROM system_policies WHERE policy_key='review_point'");
        int reviewPoint = (!policy.empty() && !policy[0]["policy_value"].empty())
                          ? std::stoi(policy[0]["policy_value"]) : 0;
        if (reviewPoint > 0) {
            db.executeUpdate("UPDATE customer_profiles SET point=point+"
                             + std::to_string(reviewPoint) + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate("INSERT INTO point_log (user_id,amount,reason) VALUES ("
                             + std::to_string(uid) + "," + std::to_string(reviewPoint) + ",'리뷰 작성 적립')");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_WRITE_REVIEW, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 207: 리뷰 목록 조회
// 요청: { token, store_id: restaurant_id }
// ──────────────────────────────────────────────────────
void CustomerHandler::handleReviewList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   restId = req.value("store_id", 0);

        auto rows = db.executeQuery(
            "SELECT rv.review_id, u.name AS author, rv.rating, rv.content, "
            "       rv.owner_reply, DATE_FORMAT(o.created_at,'%Y-%m-%d') AS created_at "
            "FROM reviews rv "
            "JOIN orders o ON o.order_id = rv.order_id "
            "JOIN users  u ON u.user_id  = rv.customer_id "
            "WHERE o.restaurant_id=" + std::to_string(restId)
            + " ORDER BY o.created_at DESC LIMIT 30");

        json reviews = json::array();
        for (auto& r : rows) {
            json rv;
            rv["id"]          = std::stoi(r["review_id"]);
            rv["author"]      = r["author"];
            rv["rating"]      = r.count("rating")  && !r["rating"].empty()  ? std::stoi(r["rating"]) : 5;
            rv["content"]     = r.count("content")     ? r["content"]     : "";
            rv["owner_reply"] = r.count("owner_reply") ? r["owner_reply"] : "";
            rv["created_at"]  = r.count("created_at")  ? r["created_at"]  : "";
            reviews.push_back(rv);
        }

        json res; res["status"] = Status::SUCCESS; res["reviews"] = reviews;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_REVIEW_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_REVIEW_LIST, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// 208: 주문 취소 (PENDING 상태만 가능)
// ──────────────────────────────────────────────────────
void CustomerHandler::handleCancelOrder(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   uid     = session->getUserID();
        int   orderId = req.value("order_id", 0);
        std::string reason = esc(req.value("reason", "고객 취소"));

        auto rows = db.executeQuery(
            "SELECT status, total_price FROM orders "
            "WHERE order_id=" + std::to_string(orderId)
            + " AND customer_id=" + std::to_string(uid));

        if (rows.empty()) { sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::NOT_FOUND, "주문 없음"); return; }

        std::string status = rows[0]["status"];
        if (status != "PENDING") {
            sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::FORBIDDEN,
                      "주문 수락 이후에는 취소 불가 (현재: " + status + ")");
            return;
        }

        // 상태 변경
        db.executeUpdate(
            "UPDATE orders SET status='CANCELED',cancel_reason='" + reason
            + "',canceled_at=NOW() WHERE order_id=" + std::to_string(orderId));

        // 상태 로그
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id,from_status,to_status,reason,changed_by) VALUES ("
            + std::to_string(orderId) + ",'PENDING','CANCELED','" + reason + "',"
            + std::to_string(uid) + ")");

        // 결제 환불 처리 (payments 상태 변경)
        db.executeUpdate(
            "UPDATE payments SET status='REFUNDED' WHERE order_id=" + std::to_string(orderId));

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::REQ_CANCEL_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::SERVER_ERROR, e.what());
    }
}

// ──────────────────────────────────────────────────────
// NTF_ORDER_STATUS (210): 서버 → 고객 Push
// OwnerHandler/RiderHandler에서 상태 변경 시 호출
// ──────────────────────────────────────────────────────
void CustomerHandler::pushOrderStatus(Session* session, int orderId,
                                       const std::string& status, const std::string& msg) {
    json ntf;
    ntf["order_id"] = orderId;
    ntf["status"]   = status; // "ACCEPTED","COOKING","DELIVERING","DONE","CANCELED"
    ntf["message"]  = msg;
    session->sendPacket(static_cast<uint8_t>(ClientType::CUSTOMER), CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
}
