// #include "AdminDBHandler.h"
// #include "MariaDBManager.h"
// #include "Protocol.h"
// #include <iostream>

// using json = nlohmann::json;

// // ============================================================
// // monitorOrders — 대기 주문 목록을 DB에서 조회한다. (1510)
// // orders 테이블에서 최근 100건을 고객명/라이더명 포함하여 반환한다.
// // ============================================================
// json AdminDBHandler::monitorOrders(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 주문 모니터링 (1510)" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();

//         // 주문 목록을 조회한다.
//         std::string q =
//             "SELECT o.order_id, o.status, "
//             "       u.login_id AS customer_name, "
//             "       IFNULL(r.login_id, '') AS rider_name "
//             "FROM orders o "
//             "LEFT JOIN users u ON o.user_id = u.user_id "
//             "LEFT JOIN users r ON o.rider_id = r.user_id "
//             "ORDER BY o.order_id DESC "
//             "LIMIT 100";

//         DBResult rows = db.executeQuery(q);

//         // 결과 JSON을 구성한다.
//         json res;
//         res["status"] = Status::SUCCESS;
//         res["orders"] = json::array();

//         for (auto& row : rows)
//         {
//             json item;
//             item["order_id"]      = row.count("order_id")      ? row.at("order_id")      : "";
//             item["status"]        = row.count("status")         ? row.at("status")         : "";
//             item["customer_name"] = row.count("customer_name")  ? row.at("customer_name")  : "";
//             item["rider_name"]    = row.count("rider_name")     ? row.at("rider_name")     : "";
//             res["orders"].push_back(item);
//         }

//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 주문 모니터링 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }

// // ============================================================
// // riderStatus — 라이더 현황을 DB에서 조회한다. (1511)
// // users 테이블에서 role='RIDER'인 사용자를 검색한다.
// // ============================================================
// json AdminDBHandler::riderStatus(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 라이더 현황 (1511)" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();

//         // 라이더 목록을 조회한다.
//         std::string q =
//             "SELECT u.user_id, u.login_id AS rider_name, "
//             "       IFNULL(u.status, 'OFFLINE') AS work_status "
//             "FROM users u "
//             "WHERE u.role = 'RIDER' "
//             "ORDER BY u.user_id";

//         DBResult rows = db.executeQuery(q);

//         // 결과 JSON을 구성한다.
//         json res;
//         res["status"] = Status::SUCCESS;
//         res["riders"] = json::array();

//         for (auto& row : rows)
//         {
//             json item;
//             item["user_id"]     = row.count("user_id")      ? row.at("user_id")     : "";
//             item["rider_name"]  = row.count("rider_name")   ? row.at("rider_name")  : "";
//             item["work_status"] = row.count("work_status")  ? row.at("work_status") : "";
//             res["riders"].push_back(item);
//         }

//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 라이더 현황 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }

// // ============================================================
// // forceDispatch — 강제 배차를 DB에서 수행한다. (1512)
// // 지정된 주문에 라이더를 배정하고 상태를 DISPATCHED로 변경한다.
// // ============================================================
// json AdminDBHandler::forceDispatch(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 강제 배차 (1512)" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();
//         int orderId = reqJson.value("order_id", 0);
//         int riderId = reqJson.value("rider_id", 0);

//         // 파라미터 검증을 수행한다.
//         if (orderId <= 0 || riderId <= 0) {
//             json res;
//             res["status"]  = Status::BAD_REQUEST;
//             res["message"] = "order_id, rider_id 필요";
//             return res;
//         }

//         // 주문에 라이더를 배정한다.
//         std::string q =
//             "UPDATE orders SET rider_id = " + std::to_string(riderId) +
//             ", status = 'DISPATCHED'"
//             " WHERE order_id = " + std::to_string(orderId);

//         json res;
//         if (db.executeUpdate(q)) {
//             res["status"]  = Status::SUCCESS;
//             res["message"] = "강제 배차 완료";
//         } else {
//             std::cout << "-------------------------" << std::endl;
//             std::cout << "관리자" << std::endl;
//             std::cout << "오류 : DB 강제 배차 실패 (order_id=" << orderId << ")" << std::endl;
//             std::cout << "-------------------------" << std::endl;
//             res["status"]  = Status::SERVER_ERROR;
//             res["message"] = "DB 업데이트 실패";
//         }
//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 강제 배차 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }

// // ============================================================
// // forceCancel — 배차 강제 취소를 DB에서 수행한다. (1513)
// // 라이더를 해제하고 주문 상태를 CANCELED로 변경한다.
// // ============================================================
// json AdminDBHandler::forceCancel(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 배차 강제 취소 (1513)" << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();
//         int orderId = reqJson.value("order_id", 0);

//         // 파라미터 검증을 수행한다.
//         if (orderId <= 0) {
//             json res;
//             res["status"]  = Status::BAD_REQUEST;
//             res["message"] = "order_id 필요";
//             return res;
//         }

//         // 라이더를 해제하고 상태를 변경한다.
//         std::string q =
//             "UPDATE orders SET rider_id = NULL, status = 'CANCELED'"
//             " WHERE order_id = " + std::to_string(orderId);

//         json res;
//         if (db.executeUpdate(q)) {
//             res["status"]  = Status::SUCCESS;
//             res["message"] = "배차 취소 완료";
//         } else {
//             std::cout << "-------------------------" << std::endl;
//             std::cout << "관리자" << std::endl;
//             std::cout << "오류 : DB 배차 취소 실패 (order_id=" << orderId << ")" << std::endl;
//             std::cout << "-------------------------" << std::endl;
//             res["status"]  = Status::SERVER_ERROR;
//             res["message"] = "DB 업데이트 실패";
//         }
//         return res;

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 배차 취소 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }

// // ============================================================
// // manageReview — 리뷰 관리를 DB에서 수행한다. (1520)
// // action: list / delete / toggle_visibility
// // ============================================================
// json AdminDBHandler::manageReview(const json& reqJson)
// {
//     std::lock_guard<std::mutex> lock(admin_db_mutex);

//     std::string action = reqJson.value("action", "list");

//     std::cout << "-------------------------" << std::endl;
//     std::cout << "관리자" << std::endl;
//     std::cout << "요청 : DB 리뷰 관리 (1520) action=" << action << std::endl;
//     std::cout << "-------------------------" << std::endl;

//     try {
//         auto& db = MariaDBManager::getInstance();

//         // ── 리뷰 목록 조회 ──
//         if (action == "list")
//         {
//             std::string q =
//                 "SELECT r.review_id, r.user_id, "
//                 "       IFNULL(u.login_id, '') AS user_name, "
//                 "       r.content, r.rating, "
//                 "       IFNULL(r.visible, 1) AS visible "
//                 "FROM reviews r "
//                 "LEFT JOIN users u ON r.user_id = u.user_id "
//                 "ORDER BY r.review_id DESC "
//                 "LIMIT 200";

//             DBResult rows = db.executeQuery(q);

//             json res;
//             res["status"]  = Status::SUCCESS;
//             res["reviews"] = json::array();

//             for (auto& row : rows)
//             {
//                 json item;
//                 item["review_id"] = row.count("review_id") ? std::stoi(row.at("review_id")) : 0;
//                 item["user_id"]   = row.count("user_name") ? row.at("user_name") : "";
//                 item["content"]   = row.count("content")   ? row.at("content")   : "";
//                 item["rating"]    = row.count("rating")     ? row.at("rating")    : "0";
//                 item["visible"]   = row.count("visible")    ? (row.at("visible") == "1") : true;
//                 res["reviews"].push_back(item);
//             }
//             return res;
//         }
//         // ── 리뷰 삭제 ──
//         else if (action == "delete")
//         {
//             int reviewId = reqJson.value("review_id", 0);
//             if (reviewId <= 0) {
//                 json res;
//                 res["status"]  = Status::BAD_REQUEST;
//                 res["message"] = "review_id 필요";
//                 return res;
//             }

//             std::string q = "DELETE FROM reviews WHERE review_id = " + std::to_string(reviewId);
//             json res;
//             if (db.executeUpdate(q)) {
//                 res["status"]  = Status::SUCCESS;
//                 res["message"] = "리뷰 삭제 완료";
//             } else {
//                 res["status"]  = Status::SERVER_ERROR;
//                 res["message"] = "삭제 실패";
//             }
//             return res;
//         }
//         // ── 보이기/숨김 토글 ──
//         else if (action == "toggle_visibility")
//         {
//             int  reviewId = reqJson.value("review_id", 0);
//             bool visible  = reqJson.value("visible", true);

//             if (reviewId <= 0) {
//                 json res;
//                 res["status"]  = Status::BAD_REQUEST;
//                 res["message"] = "review_id 필요";
//                 return res;
//             }

//             std::string q =
//                 "UPDATE reviews SET visible = " + std::string(visible ? "1" : "0") +
//                 " WHERE review_id = " + std::to_string(reviewId);

//             json res;
//             if (db.executeUpdate(q)) {
//                 res["status"]  = Status::SUCCESS;
//                 res["message"] = visible ? "리뷰 보이기" : "리뷰 숨김";
//             } else {
//                 res["status"]  = Status::SERVER_ERROR;
//                 res["message"] = "업데이트 실패";
//             }
//             return res;
//         }
//         else
//         {
//             json res;
//             res["status"]  = Status::BAD_REQUEST;
//             res["message"] = "알 수 없는 action: " + action;
//             return res;
//         }

//     } catch (const std::exception& e) {
//         std::cout << "-------------------------" << std::endl;
//         std::cout << "관리자" << std::endl;
//         std::cout << "오류 : DB 리뷰 관리 예외 (" << e.what() << ")" << std::endl;
//         std::cout << "-------------------------" << std::endl;
//         json res;
//         res["status"]  = Status::SERVER_ERROR;
//         res["message"] = e.what();
//         return res;
//     }
// }