#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"

#include "CommonDB.h"

using json = nlohmann::json;
void CustomerHandler::handleCreateOrder(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);

        int   uid     = getUserIdByFd(session->getFd()); // Session 대신 BaseHandler 로직 사용
        int   storeID = req.value("store_id",   0);
        bool  isDel   = req.value("is_delivery", true);
        int   usePoint= req.value("use_point",  0);
        std::string delAddr = req.value("delivery_address", "");

        // ★ 1. 로그인 유저 검증 및 파라미터 유효성 검사 강화
        if (uid <= 0 || storeID <= 0) {
            sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "필수 파라미터 누락 또는 비로그인");
            return;
        }

        // 해당 가게의 배달비 조회
        auto storeRows = db.executeQuery("SELECT base_delivery_fee FROM restaurants WHERE restaurant_id = " + std::to_string(storeID));
        int actualDeliveryFee = 0;
        if (!storeRows.empty()) {
            actualDeliveryFee = std::stoi(storeRows[0].at("base_delivery_fee"));
        }

        int totalPrice = 0;
        for (auto& it : req["items"]) {
            int qty   = it.value("qty", 1);
            int price = it.value("price", 0);
            totalPrice += price * qty;
            if (it.contains("options"))
                for (auto& opt : it["options"])
                    totalPrice += opt.value("extra_price", 0) * qty;
        }

        // 배달비 추가 로직
        if (isDel) {
            totalPrice += actualDeliveryFee; 
        }
        // std::cout << "조회된 배달비: " << actualDeliveryFee << std::endl;

        if (usePoint > 0) {
            auto pr = db.executeQuery("SELECT point FROM customer_profiles WHERE user_id=" + std::to_string(uid));
            int held = (!pr.empty() && !pr[0].at("point").empty()) ? std::stoi(pr[0].at("point")) : 0;
            if (usePoint > held) { sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "포인트 부족"); return; }
            totalPrice -= usePoint;
        }

        std::string method = isDel ? "DELIVERY" : "PICKUP";

        // orders INSERT - status 기본값 PENDING (결제 완료 전 접수 상태)
        bool ok = db.executeUpdate(
            "INSERT INTO orders (customer_id, restaurant_id, status, total_price, delivery_method, delivery_address) VALUES ("
            + std::to_string(uid) + "," + std::to_string(storeID) + ",'PENDING'," + std::to_string(totalPrice) + ",'"
            + method + "','" + CommonDB::getInstance().escape(delAddr) + "')");
        if (!ok) { sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, "주문 생성 실패"); return; }
        
        uint64_t orderID = db.getLastInsertId();

        for (auto& it : req["items"]) {
            int menuID = it.value("menu_id", 0);
            int qty    = it.value("qty",     1);
            int price  = it.value("price",   0);

            // menu_name 스냅샷 조회
            std::string menuName = "";
            auto mnRows = db.executeQuery(
                "SELECT menu_name FROM menus WHERE menu_id=" + std::to_string(menuID));
            if (!mnRows.empty()) menuName = CommonDB::getInstance().escape(mnRows[0].at("menu_name"));

            db.executeUpdate(
                "INSERT INTO order_items "
                "(order_id, menu_id, menu_name, price_at_order, quantity) VALUES ("
                + std::to_string(orderID) + "," + std::to_string(menuID)
                + ",'" + menuName + "'," + std::to_string(price)
                + "," + std::to_string(qty) + ")");
            uint64_t oiID = db.getLastInsertId();

            if (it.contains("options")) {
                for (auto& opt : it["options"]) {
                    int optItemID  = opt.value("option_item_id", 0);
                    int extraPrice = opt.value("extra_price",    0);

                    // option_name 스냅샷 조회
                    std::string optName = "";
                    auto onRows = db.executeQuery(
                        "SELECT option_name FROM option_items WHERE option_item_id="
                        + std::to_string(optItemID));
                    if (!onRows.empty()) optName = CommonDB::getInstance().escape(onRows[0].at("option_name"));

                    if (optItemID)
                        db.executeUpdate(
                            "INSERT INTO order_item_options "
                            "(order_item_id, option_item_id, option_name, extra_price) VALUES ("
                            + std::to_string(oiID) + "," + std::to_string(optItemID)
                            + ",'" + optName + "'," + std::to_string(extraPrice) + ")");
                }
            }
        }

        if (usePoint > 0) {
            db.executeUpdate("UPDATE customer_profiles SET point = point - " + std::to_string(usePoint) + " WHERE user_id=" + std::to_string(uid));
            // point_log: amount, reason (order_id 컬럼 없음)
            db.executeUpdate("INSERT INTO point_log (user_id, amount, reason) VALUES ("
                + std::to_string(uid) + ",-" + std::to_string(usePoint) + ",'ORDER_USE')");
        }

        // 주문 상태 로그 (PENDING = 기본 초기 상태)
        db.executeUpdate("INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderID) + ",'','PENDING'," + std::to_string(uid) + ")");

        json res;
        res["status"]            = Status::SUCCESS;
        res["order_id"]          = (int)orderID;
        res["delivery_fee"]      = actualDeliveryFee; // ★ 추가: 계산된 배달비를 응답에 포함
        res["estimated_minutes"] = 30;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_CREATE_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleOrderHistory(Session* session, const std::string&) {
    try {
        auto& db = MariaDBManager::getInstance();
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) {
            sendError(session, CmdCustomer::REQ_ORDER_HISTORY,
                      Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        // ★ delivery_method, delivery_address 추가
        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "o.total_price, o.status, o.created_at AS order_time, "
            "o.delivery_method, o.delivery_address, "
            "r.base_delivery_fee AS delivery_fee " // ★ AS delivery_fee 추가!
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.customer_id=" + std::to_string(uid) +
            " ORDER BY o.created_at DESC LIMIT 50");

        // ★ 1. 클라이언트(OrderInfo.h)의 enum 숫자에 맞게 매핑 수정
        auto statusToInt = [](const std::string& s) {
            if (s == "PENDING" || s == "WAITING")           return 0; // STATUS_WAITING (접수 대기)
            if (s == "ACCEPTED" || s == "COOKING" || s == "PREPARING") return 1; // STATUS_PREPARING (조리 중)
            if (s == "WAITING_PICKUP" || s == "DELIVERING") return 2; // STATUS_DELIVERING (배달 중)
            if (s == "DONE" || s == "COMPLETE")             return 3; // STATUS_COMPLETE (배달 완료)
            if (s == "CANCELED" || s == "REJECTED")         return 4; // STATUS_CANCELED (주문 취소)
            return 0; // 기본값
        };

        // auto statusToInt = [](const std::string& s) {
        //     if (s == "CANCELED" || s == "REJECTED") return 0;
        //     if (s == "PENDING")                     return 1;
        //     if (s == "ACCEPTED" || s == "COOKING")  return 2;
        //     if (s == "WAITING_PICKUP" || s == "DELIVERING") return 3;
        //     if (s == "DONE")                        return 4;
        //     return 0;
        // };

        json orders = json::array();
        for (auto& r : rows) {
            int oid = std::stoi(r.at("order_id"));
            json o;

            o["order_id"]        = oid;
            o["store_name"]      = r.at("store_name");
            o["total_price"]     = std::stoi(r.count("total_price") ? r.at("total_price") : "0");
            o["delivery_fee"]    = std::stoi(r.count("delivery_fee") ? r.at("delivery_fee") : "0"); // ★ 추가
            o["status"]          = statusToInt(r.count("status") ? r.at("status") : "");
            o["order_time"]      = r.count("order_time")      ? r.at("order_time")      : "";
            o["delivery_method"] = r.count("delivery_method") ? r.at("delivery_method") : "";
            o["delivery_address"]= r.count("delivery_address")? r.at("delivery_address"): "";

            // o["order_id"]        = std::to_string(oid);  // 클라이언트 OrderInfo.h에서 std::string으로 선언됨
            // o["store_name"]      = r.at("store_name");
            // o["total_payment"]   = std::stoi(r.count("total_price") ? r.at("total_price") : "0"); // total_price -> total_payment
            // o["status"]          = statusToInt(r.count("status") ? r.at("status") : "");
            // o["order_datetime"]  = r.count("order_time") ? r.at("order_time") : "";               // order_time -> order_datetime
            // o["delivery_method"] = r.count("delivery_method") ? r.at("delivery_method") : "";
            // o["delivery_address"]= r.count("delivery_address")? r.at("delivery_address"): "";

            // ★ 주문 메뉴 + 옵션 조회
            auto itemRows = db.executeQuery(
                "SELECT oi.order_item_id, oi.menu_name, oi.quantity, oi.price_at_order "
                "FROM order_items oi "
                "WHERE oi.order_id=" + std::to_string(oid));

            json items = json::array();
            for (auto& it : itemRows) {
                int oiID = std::stoi(it.at("order_item_id"));
                json item;
                item["menu_name"] = it.at("menu_name");
                item["quantity"]  = std::stoi(it.count("quantity")       ? it.at("quantity")       : "1");
                item["price"]     = std::stoi(it.count("price_at_order") ? it.at("price_at_order") : "0");

                // ★ 옵션 조회
                auto optRows = db.executeQuery(
                    "SELECT option_name, extra_price "
                    "FROM order_item_options "
                    "WHERE order_item_id=" + std::to_string(oiID));

                json opts = json::array();
                for (auto& op : optRows) {
                    json opt;
                    opt["option_name"] = op.at("option_name");
                    opt["extra_price"] = std::stoi(op.count("extra_price") ? op.at("extra_price") : "0");
                    opts.push_back(opt);
                }
                item["options"] = opts;
                items.push_back(item);
            }
            o["items"] = items;
            orders.push_back(o);
        }

        json res;
        res["status"] = Status::SUCCESS;
        res["orders"] = orders;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_ORDER_HISTORY, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_ORDER_HISTORY,
                  Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleOrderDetail(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        
        // 1. 세션에서 유저 ID 가져오기 및 검증 (방금 추가하신 핵심 보안 로직!)
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) { 
            sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::UNAUTHORIZED, "로그인 필요"); 
            return; 
        } 

        // 2. 요청 바디에서 order_id 파싱 (빠졌던 부분 복구)
        json  req   = json::parse(body);
        int   orderID = req.value("order_id", 0);

        // 3. 본인(uid)의 주문(orderID)이 맞는지 쿼리에서 안전하게 조회
        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, r.phone AS store_phone, r.base_delivery_fee, "
            "o.total_price, o.status, o.created_at AS order_time, o.delivery_method, o.delivery_address "
            "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderID) + " AND o.customer_id=" + std::to_string(uid));

        if (rows.empty()) { 
            // 내 주문이 아니거나 없는 주문이면 에러 처리
            sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::NOT_FOUND, "주문 없음 또는 권한 없음"); 
            return; 
        }

        auto& r = rows[0];
        json res;
        res["status"]       = Status::SUCCESS;
        res["order_id"]     = orderID;
        res["store_name"]   = r.at("store_name");
        res["store_phone"]  = r.count("store_phone")  ? r.at("store_phone")  : "";
        res["total_price"]  = std::stoi(r.count("total_price") ? r.at("total_price") : "0");
        res["delivery_fee"] = std::stoi(r.count("delivery_fee") ? r.at("delivery_fee") : "0"); // ★ 추가
        res["order_status"] = r.count("status") ? r.at("status") : "";
        res["order_time"]   = r.count("order_time") ? r.at("order_time") : "";

        auto itemRows = db.executeQuery(
            "SELECT m.menu_name AS name, oi.quantity, oi.price_at_order AS price "
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

        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_ORDER_DETAIL, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handlePayment(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        
        // ★ 1. 로그인 유저 검증
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) { 
            sendError(session, CmdCustomer::REQ_PAYMENT, Status::UNAUTHORIZED, "로그인 필요"); 
            return; 
        }

        json  req = json::parse(body);
        int orderID  = req.value("order_id", 0);
        int pmID     = req.value("payment_method_id", 0);
        int amount   = req.value("amount", 0);

        // ★ 2. 내 주문이 맞는지 확인 (타인 주문 결제 방지)
        auto rows = db.executeQuery("SELECT status FROM orders WHERE order_id=" + std::to_string(orderID) + " AND customer_id=" + std::to_string(uid));
        if (rows.empty()) { 
            sendError(session, CmdCustomer::REQ_PAYMENT, Status::NOT_FOUND, "주문 없음 또는 권한 없음"); 
            return; 
        }

        // payments: order_id, method_type, total_amount, status
        std::string pmType = "CARD";
        if (pmID > 0) {
            auto pmRows = db.executeQuery(
                "SELECT method_type FROM payment_methods WHERE payment_method_id="
                + std::to_string(pmID));
            if (!pmRows.empty()) pmType = pmRows[0].at("method_type");
        }
        bool ok = db.executeUpdate(
            "INSERT INTO payments (order_id, method_type, total_amount, status) VALUES ("
            + std::to_string(orderID) + ",'" + pmType + "',"
            + std::to_string(amount) + ",'SUCCESS')");

        if (ok) db.executeUpdate("UPDATE orders SET status='PENDING' WHERE order_id=" + std::to_string(orderID));

        json res; res["status"] = ok ? Status::SUCCESS : Status::SERVER_ERROR;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_PAYMENT, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_PAYMENT, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleCancelOrder(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        
        // ★ 1. 로그인 유저 검증 추가
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) { 
            sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::UNAUTHORIZED, "로그인 필요"); 
            return; 
        }

        json  req = json::parse(body);
        int   orderID= req.value("order_id", 0);

        // 2. 이 부분은 이미 AND customer_id=uid 로 완벽하게 짜셨습니다!
        auto rows = db.executeQuery("SELECT status FROM orders WHERE order_id=" + std::to_string(orderID) + " AND customer_id=" + std::to_string(uid));
        if (rows.empty()) { sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::NOT_FOUND, "주문 없음"); return; }

        std::string st = rows[0].at("status");
        if (st != "PENDING") {
            sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::FORBIDDEN, "주문접수 상태에서만 취소 가능합니다");
            return;
        }

        db.executeUpdate("UPDATE orders SET status='CANCELED' WHERE order_id=" + std::to_string(orderID));

        // point_log에서 ORDER_USE 내역 찾아 포인트 환불
        // point_log 스키마: user_id, amount, reason (order_id 컬럼 없음 → user+reason으로 최근 것 조회)
        auto plRows = db.executeQuery(
            "SELECT amount FROM point_log WHERE user_id=" + std::to_string(uid)
            + " AND reason='ORDER_USE' ORDER BY log_id DESC LIMIT 1");
        if (!plRows.empty()) {
            int usedPoint = std::abs(std::stoi(plRows[0].at("amount")));
            db.executeUpdate(
                "UPDATE customer_profiles SET point = point + "
                + std::to_string(usedPoint) + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate(
                "INSERT INTO point_log (user_id, amount, reason) VALUES ("
                + std::to_string(uid) + "," + std::to_string(usedPoint) + ",'CANCEL_REFUND')");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_CANCEL_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::SERVER_ERROR, e.what());
    }
}