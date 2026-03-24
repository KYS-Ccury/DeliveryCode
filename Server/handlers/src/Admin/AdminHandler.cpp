// // AdminHandler.cpp – DB 연동 완성본
// #include "AdminHandler.h"
// #include "ChatHandler.h"
// #include "RiderHandler.h"
// #include "CustomerHandler.h"
// #include "Session.h"
// #include "MariaDBManager.h"
// #include "EpollServer.h"
// #include "Protocol.h"
// #include <nlohmann/json.hpp>
// #include <iostream>

// using json = nlohmann::json;

// static void sendErr(Session* s, uint16_t proto, uint16_t code, const std::string& msg) {
//     json r; r["status"] = code; r["message"] = msg;
//     s->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), proto, r.dump());
// }
// static std::string esc(const std::string& s) {
//     std::string o; for (char c : s) { if (c=='\''||c=='\\'||c=='"') o+='\\'; o+=c; } return o;
// }

// void AdminHandler::registerSession  (int fd, int adminId) { ChatHandler::registerAdmin(fd, adminId); }
// void AdminHandler::unregisterSession(int fd)               { ChatHandler::unregisterAdmin(fd); }

// void AdminHandler::process(Session* session, uint16_t protocol, const std::string& body) {
//     std::cout << "[AdminHandler] Protocol: " << protocol << "\n";
//     switch (protocol) {
//         case CmdCommon::REQ_LOGIN: handleLogin(session, body); break;
//         case CmdCommon::REQ_LOGOUT:
//             ChatHandler::unregisterAdmin(session->getFd());
//             { json r; r["status"]=Status::SUCCESS;
//               session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN),CmdCommon::REQ_LOGOUT,r.dump()); }
//             break;
//         case CmdAdmin::REQ_MONITOR_ORDERS:  handleMonitorOrders (session, body); break;
//         case CmdAdmin::REQ_RIDER_STATUS:    handleRiderStatus   (session, body); break;
//         case CmdAdmin::REQ_FORCE_DISPATCH:  handleForceDispatch (session, body); break;
//         case CmdAdmin::REQ_FORCE_CANCEL:    handleForceCancel   (session, body); break;
//         case CmdAdmin::REQ_SETTLEMENT_LIST: handleSettlementList(session, body); break;
//         case CmdAdmin::REQ_SETTLEMENT_CONF: handleSettlementConf(session, body); break;
//         case CmdAdmin::REQ_MANAGE_REVIEW:   handleManageReview  (session, body); break;
//         case CmdChat::REQ_CREATE_ROOM:
//         case CmdChat::REQ_SEND_MSG:
//         case CmdChat::REQ_GET_MSGS:
//             ChatHandler::process(session, protocol, body, ClientType::ADMIN);
//             break;
//         default:
//             std::cerr << "[AdminHandler] 알 수 없는 프로토콜: " << protocol << "\n";
//             sendErr(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
//     }
// }

// // ─────────────────────────────────────────────────
// // 510: 대기 주문 모니터링 (실시간 현황판)
// // ─────────────────────────────────────────────────
// void AdminHandler::handleMonitorOrders(Session* session, const std::string&) {
//     try {
//         auto& db = MariaDBManager::getInstance();

//         // 오늘 주문 수, 대기 중 주문 수, 활성 라이더 수
//         auto statsRows = db.executeQuery(
//             "SELECT "
//             " (SELECT COUNT(*) FROM orders WHERE DATE(created_at)=CURDATE()) AS today_orders, "
//             " (SELECT COUNT(*) FROM orders WHERE status IN ('PENDING','ACCEPTED','COOKING','WAITING_PICKUP','DELIVERING')) AS active_orders, "
//             " (SELECT COUNT(*) FROM rider_profiles WHERE is_working=TRUE AND is_online=TRUE) AS active_riders");

//         // WAITING_PICKUP 배차 대기 주문 목록
//         auto waitRows = db.executeQuery(
//             "SELECT o.order_id, r.restaurant_name AS store_name, r.address AS pickup_addr, "
//             "       o.delivery_address AS dest_addr, r.base_delivery_fee, o.total_price, "
//             "       TIMESTAMPDIFF(SECOND, o.created_at, NOW()) AS wait_sec "
//             "FROM orders o "
//             "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
//             "WHERE o.status='WAITING_PICKUP' AND o.rider_id IS NULL "
//             "ORDER BY o.created_at ASC LIMIT 50");

//         json res;
//         res["status"] = Status::SUCCESS;
//         if (!statsRows.empty()) {
//             res["today_orders"]  = std::stoi(statsRows[0].count("today_orders")  ? statsRows[0].at("today_orders")  : "0");
//             res["active_orders"] = std::stoi(statsRows[0].count("active_orders") ? statsRows[0].at("active_orders") : "0");
//             res["active_riders"] = std::stoi(statsRows[0].count("active_riders") ? statsRows[0].at("active_riders") : "0");
//         }

//         json waiting = json::array();
//         for (auto& r : waitRows) {
//             json w;
//             w["order_id"]     = std::stoi(r.at("order_id"));
//             w["store_name"]   = r.at("store_name");
//             w["pickup_addr"]  = r.count("pickup_addr") ? r.at("pickup_addr") : "";
//             w["dest_addr"]    = r.count("dest_addr")   ? r.at("dest_addr")   : "";
//             w["delivery_fee"] = std::stoi(r.count("base_delivery_fee") ? r.at("base_delivery_fee") : "0");
//             w["total_price"]  = std::stoi(r.count("total_price") ? r.at("total_price") : "0");
//             w["wait_sec"]     = std::stoi(r.count("wait_sec") ? r.at("wait_sec") : "0");
//             waiting.push_back(w);
//         }
//         res["waiting_orders"] = waiting;

//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_MONITOR_ORDERS, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_MONITOR_ORDERS, Status::SERVER_ERROR, e.what());
//     }
// }

// // ─────────────────────────────────────────────────
// // 511: 라이더 현황 조회
// // ─────────────────────────────────────────────────
// void AdminHandler::handleRiderStatus(Session* session, const std::string&) {
//     try {
//         auto& db = MariaDBManager::getInstance();

//         auto rows = db.executeQuery(
//             "SELECT u.user_id, u.name, u.phone, rp.vehicle_type, "
//             "       rp.is_working, rp.is_online, rp.is_accepting, rp.is_admin_blocked "
//             "FROM users u "
//             "JOIN rider_profiles rp ON rp.user_id = u.user_id "
//             "WHERE u.role='RIDER' AND u.status='ACTIVE' "
//             "ORDER BY rp.is_online DESC, rp.is_working DESC");

//         json riders = json::array();
//         for (auto& r : rows) {
//             json rd;
//             rd["rider_id"]       = std::stoi(r.at("user_id"));
//             rd["name"]           = r.at("name");
//             rd["phone"]          = r.count("phone")        ? r.at("phone")        : "";
//             rd["vehicle_type"]   = r.count("vehicle_type") ? r.at("vehicle_type") : "";
//             rd["is_working"]     = (r.at("is_working")   == "1");
//             rd["is_online"]      = (r.at("is_online")    == "1");
//             rd["is_accepting"]   = (r.at("is_accepting") == "1");
//             rd["is_blocked"]     = (r.at("is_admin_blocked") == "1");
//             riders.push_back(rd);
//         }

//         json res; res["status"] = Status::SUCCESS; res["riders"] = riders;
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_RIDER_STATUS, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_RIDER_STATUS, Status::SERVER_ERROR, e.what());
//     }
// }

// // ─────────────────────────────────────────────────
// // 512: 강제 배차 (관리자가 라이더 지정)
// // 요청: { order_id, rider_id }
// // ─────────────────────────────────────────────────
// void AdminHandler::handleForceDispatch(Session* session, const std::string& body) {
//     try {
//         auto& db  = MariaDBManager::getInstance();
//         json  req = json::parse(body);
//         int   orderID  = req.value("order_id",  0);
//         int   riderID  = req.value("rider_id",  0);

//         if (!orderID || !riderID) { sendErr(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::BAD_REQUEST, "파라미터 누락"); return; }

//         // 주문 상태 DELIVERING으로, rider_id 지정
//         bool ok = db.executeUpdate(
//             "UPDATE orders SET rider_id=" + std::to_string(riderID)
//             + ", status='DELIVERING' WHERE order_id=" + std::to_string(orderID)
//             + "  AND status='WAITING_PICKUP'");

//         if (!ok) { sendErr(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::NOT_FOUND, "배차 가능한 주문 없음"); return; }

//         db.executeUpdate(
//             "INSERT INTO dispatch_logs (order_id, rider_id, result) VALUES ("
//             + std::to_string(orderID) + "," + std::to_string(riderID) + ",'FORCE_DISPATCH')");

//         // 라이더에게 NTF_NEW_DISPATCH Push
//         int riderFd = RiderHandler::getInstance().getFdByUserId(riderID);
//         if (riderFd > 0 && EpollServer::s_instance) {
//             auto rs = EpollServer::s_instance->getSession(riderFd);
//             if (rs) {
//                 auto detailRows = db.executeQuery(
//                     "SELECT r.restaurant_name, r.address, o.delivery_address, r.base_delivery_fee "
//                     "FROM orders o JOIN restaurants r ON r.restaurant_id=o.restaurant_id "
//                     "WHERE o.order_id=" + std::to_string(orderID));
//                 if (!detailRows.empty()) {
//                     auto& d = detailRows[0];
//                     RiderHandler::getInstance().pushDispatch(riderFd, orderID,
//                         d.at("restaurant_name"),
//                         d.count("address") ? d.at("address") : "",
//                         d.count("delivery_address") ? d.at("delivery_address") : "",
//                         std::stoi(d.count("base_delivery_fee") ? d.at("base_delivery_fee") : "0"));
//                 }
//             }
//         }

//         json res; res["status"] = Status::SUCCESS; res["message"] = "강제 배차 완료";
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_FORCE_DISPATCH, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::SERVER_ERROR, e.what());
//     }
// }

// // ─────────────────────────────────────────────────
// // 513: 배차 강제 취소
// // 요청: { order_id, reason }
// // ─────────────────────────────────────────────────
// void AdminHandler::handleForceCancel(Session* session, const std::string& body) {
//     try {
//         auto& db  = MariaDBManager::getInstance();
//         json  req = json::parse(body);
//         int   orderID = req.value("order_id", 0);
//         std::string reason = req.value("reason", "관리자 강제 취소");

//         // 주문 취소
//         db.executeUpdate(
//             "UPDATE orders SET status='CANCELED', rider_id=NULL WHERE order_id=" + std::to_string(orderID));

//         // 고객에게 Push
//         auto custRows = db.executeQuery(
//             "SELECT customer_id FROM orders WHERE order_id=" + std::to_string(orderID));
//         if (!custRows.empty() && EpollServer::s_instance) {
//             int custID = std::stoi(custRows[0].at("customer_id"));
//             auto cs = EpollServer::s_instance->getSessionByUserID(custID);
//             if (cs)
//                 CustomerHandler::getInstance().pushOrderStatus(cs, orderID, 0, "관리자에 의해 주문이 취소되었습니다: " + reason);
//         }

//         json res; res["status"] = Status::SUCCESS;
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_FORCE_CANCEL, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_FORCE_CANCEL, Status::SERVER_ERROR, e.what());
//     }
// }

// // ─────────────────────────────────────────────────
// // 500: 정산 목록 조회 (1일 단위)
// // ─────────────────────────────────────────────────
// void AdminHandler::handleSettlementList(Session* session, const std::string& body) {
//     try {
//         auto& db = MariaDBManager::getInstance();
//         json  req = body.empty() ? json::object() : json::parse(body);
//         std::string date = req.value("date", ""); // "2026-03-24"

//         std::string q =
//             "SELECT s.settlement_id, r.restaurant_name, s.settle_date, "
//             "       s.total_sales, s.commission, s.payout_amount, s.is_confirmed "
//             "FROM settlements s "
//             "JOIN restaurants r ON r.restaurant_id = s.restaurant_id ";
//         if (!date.empty()) q += "WHERE s.settle_date='" + esc(date) + "' ";
//         q += "ORDER BY s.settle_date DESC, r.restaurant_name";

//         auto rows = db.executeQuery(q);
//         json list = json::array();
//         for (auto& r : rows) {
//             json s;
//             s["id"]          = std::stoi(r.at("settlement_id"));
//             s["store_name"]  = r.at("restaurant_name");
//             s["date"]        = r.at("settle_date");
//             s["total_sales"] = std::stoi(r.count("total_sales")   ? r.at("total_sales")   : "0");
//             s["commission"]  = std::stoi(r.count("commission")    ? r.at("commission")    : "0");
//             s["payout"]      = std::stoi(r.count("payout_amount") ? r.at("payout_amount") : "0");
//             s["confirmed"]   = (r.at("is_confirmed") == "1");
//             list.push_back(s);
//         }

//         json res; res["status"] = Status::SUCCESS; res["settlements"] = list;
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_SETTLEMENT_LIST, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_SETTLEMENT_LIST, Status::SERVER_ERROR, e.what());
//     }
// }

// // ─────────────────────────────────────────────────
// // 502: 정산 확정
// // ─────────────────────────────────────────────────
// void AdminHandler::handleSettlementConf(Session* session, const std::string& body) {
//     try {
//         auto& db  = MariaDBManager::getInstance();
//         json  req = json::parse(body);
//         int   sid = req.value("settlement_id", 0);

//         db.executeUpdate(
//             "UPDATE settlements SET is_confirmed=TRUE, confirmed_at=NOW() "
//             "WHERE settlement_id=" + std::to_string(sid));

//         json res; res["status"] = Status::SUCCESS;
//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_SETTLEMENT_CONF, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_SETTLEMENT_CONF, Status::SERVER_ERROR, e.what());
//     }
// }

// // ─────────────────────────────────────────────────
// // 520: 리뷰/사용자 관리 (비공개 처리 또는 계정 정지)
// // ─────────────────────────────────────────────────
// void AdminHandler::handleManageReview(Session* session, const std::string& body) {
//     try {
//         auto& db  = MariaDBManager::getInstance();
//         json  req = json::parse(body);
//         std::string action = req.value("action", "");

//         json res; res["status"] = Status::SUCCESS;

//         if (action == "DELETE_REVIEW") {
//             int reviewID = req.value("review_id", 0);
//             db.executeUpdate("DELETE FROM reviews WHERE review_id=" + std::to_string(reviewID));
//             res["message"] = "리뷰 삭제 완료";

//         } else if (action == "BAN_USER") {
//             int targetID = req.value("target_user_id", 0);
//             db.executeUpdate(
//                 "UPDATE users SET status='SUSPENDED' WHERE user_id=" + std::to_string(targetID));
//             res["message"] = "사용자 정지 완료";

//         } else if (action == "BLOCK_RIDER") {
//             int riderID = req.value("rider_id", 0);
//             db.executeUpdate(
//                 "UPDATE rider_profiles SET is_admin_blocked=TRUE WHERE user_id=" + std::to_string(riderID));
//             res["message"] = "라이더 차단 완료";

//         } else {
//             res["status"]  = Status::BAD_REQUEST;
//             res["message"] = "알 수 없는 action: " + action;
//         }

//         session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), CmdAdmin::REQ_MANAGE_REVIEW, res.dump());

//     } catch (const std::exception& e) {
//         sendErr(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::SERVER_ERROR, e.what());
//     }
// }
