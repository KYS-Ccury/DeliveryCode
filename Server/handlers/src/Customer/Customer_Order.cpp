#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"

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

        if (!uid || !storeID) {
            sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "필수 파라미터 누락 또는 비로그인");
            return;
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

        if (usePoint > 0) {
            auto pr = db.executeQuery("SELECT point FROM customer_profiles WHERE user_id=" + std::to_string(uid));
            int held = (!pr.empty() && !pr[0].at("point").empty()) ? std::stoi(pr[0].at("point")) : 0;
            if (usePoint > held) { sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::BAD_REQUEST, "포인트 부족"); return; }
            totalPrice -= usePoint;
        }

        std::string method = isDel ? "DELIVERY" : "PICKUP";

        bool ok = db.executeUpdate(
            "INSERT INTO orders (customer_id, restaurant_id, status, total_price, delivery_method, delivery_address) VALUES ("
            + std::to_string(uid) + "," + std::to_string(storeID) + ",'PENDING'," + std::to_string(totalPrice) + ",'"
            + method + "','" + escapeStr(delAddr) + "')");
        if (!ok) { sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, "주문 생성 실패"); return; }
        
        uint64_t orderID = db.getLastInsertId();

        for (auto& it : req["items"]) {
            int menuID = it.value("menu_id", 0);
            int qty    = it.value("qty",     1);
            int price  = it.value("price",   0);

            db.executeUpdate("INSERT INTO order_items (order_id, menu_id, quantity, unit_price) VALUES ("
                + std::to_string(orderID) + "," + std::to_string(menuID) + "," + std::to_string(qty) + "," + std::to_string(price) + ")");
            uint64_t oiID = db.getLastInsertId();

            if (it.contains("options")) {
                for (auto& opt : it["options"]) {
                    int optItemID  = opt.value("option_item_id", 0);
                    int extraPrice = opt.value("extra_price",    0);
                    if (optItemID)
                        db.executeUpdate("INSERT INTO order_item_options (order_item_id, option_item_id, extra_price) VALUES ("
                            + std::to_string(oiID) + "," + std::to_string(optItemID) + "," + std::to_string(extraPrice) + ")");
                }
            }
        }

        if (usePoint > 0) {
            db.executeUpdate("UPDATE customer_profiles SET point = point - " + std::to_string(usePoint) + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate("INSERT INTO point_log (user_id, change_amount, reason, order_id) VALUES ("
                + std::to_string(uid) + ",-" + std::to_string(usePoint) + ",'ORDER_USE'," + std::to_string(orderID) + ")");
        }

        db.executeUpdate("INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) VALUES ("
            + std::to_string(orderID) + ",'','PENDING'," + std::to_string(uid) + ")");

        json res; res["status"] = Status::SUCCESS; res["order_id"] = (int)orderID;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_CREATE_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_CREATE_ORDER, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleOrderHistory(Session* session, const std::string&) {
    try {
        auto& db  = MariaDBManager::getInstance();
        int   uid = getUserIdByFd(session->getFd());
        if (!uid) { sendError(session, CmdCustomer::REQ_ORDER_HISTORY, Status::UNAUTHORIZED, "로그인 필요"); return; }

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, o.total_price, o.status, o.created_at AS order_time "
            "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.customer_id=" + std::to_string(uid) + " ORDER BY o.created_at DESC LIMIT 50");

        auto statusToInt = [](const std::string& s) {
            if (s == "CANCELED" || s == "REJECTED") return 0;
            if (s == "PENDING") return 1;
            if (s == "ACCEPTED" || s == "COOKING") return 2;
            if (s == "WAITING_PICKUP" || s == "DELIVERING") return 3;
            if (s == "DONE") return 4;
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
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_ORDER_HISTORY, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_ORDER_HISTORY, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleOrderDetail(Session* session, const std::string& body) {
    try {
        auto& db    = MariaDBManager::getInstance();
        json  req   = json::parse(body);
        int   orderID = req.value("order_id", 0);

        auto rows = db.executeQuery(
            "SELECT o.order_id, r.restaurant_name AS store_name, r.phone AS store_phone, "
            "o.total_price, o.status, o.created_at AS order_time, o.delivery_method, o.delivery_address "
            "FROM orders o JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id=" + std::to_string(orderID));

        if (rows.empty()) { sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::NOT_FOUND, "주문 없음"); return; }

        auto& r = rows[0];
        json res;
        res["status"]       = Status::SUCCESS;
        res["order_id"]     = orderID;
        res["store_name"]   = r.at("store_name");
        res["store_phone"]  = r.count("store_phone")  ? r.at("store_phone")  : "";
        res["total_price"]  = std::stoi(r.count("total_price") ? r.at("total_price") : "0");
        res["order_status"] = r.count("status") ? r.at("status") : "";
        res["order_time"]   = r.count("order_time") ? r.at("order_time") : "";

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

        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_ORDER_DETAIL, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_ORDER_DETAIL, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handlePayment(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int orderID  = req.value("order_id", 0);
        int pmID     = req.value("payment_method_id", 0);
        int amount   = req.value("amount", 0);

        bool ok = db.executeUpdate("INSERT INTO payments (order_id, payment_method_id, amount, paid_at) VALUES ("
            + std::to_string(orderID) + "," + std::to_string(pmID) + "," + std::to_string(amount) + ",NOW())");

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
        json  req = json::parse(body);
        int   uid    = getUserIdByFd(session->getFd());
        int   orderID= req.value("order_id", 0);

        auto rows = db.executeQuery("SELECT status FROM orders WHERE order_id=" + std::to_string(orderID) + " AND customer_id=" + std::to_string(uid));
        if (rows.empty()) { sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::NOT_FOUND, "주문 없음"); return; }

        std::string st = rows[0].at("status");
        if (st != "PENDING") {
            sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::FORBIDDEN, "주문접수 상태에서만 취소 가능합니다");
            return;
        }

        db.executeUpdate("UPDATE orders SET status='CANCELED' WHERE order_id=" + std::to_string(orderID));

        auto plRows = db.executeQuery("SELECT change_amount FROM point_log WHERE order_id=" + std::to_string(orderID) + " AND reason='ORDER_USE'");
        if (!plRows.empty()) {
            int usedPoint = std::abs(std::stoi(plRows[0].at("change_amount")));
            db.executeUpdate("UPDATE customer_profiles SET point = point + " + std::to_string(usedPoint) + " WHERE user_id=" + std::to_string(uid));
            db.executeUpdate("INSERT INTO point_log (user_id, change_amount, reason, order_id) VALUES ("
                + std::to_string(uid) + "," + std::to_string(usedPoint) + ",'CANCEL_REFUND'," + std::to_string(orderID) + ")");
        }

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_CANCEL_ORDER, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_CANCEL_ORDER, Status::SERVER_ERROR, e.what());
    }
}