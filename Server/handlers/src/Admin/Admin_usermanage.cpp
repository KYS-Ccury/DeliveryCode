#include "AdminHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

// ============================================================
// 510: handleOrderMonitor — 대기 주문 목록을 조회한다.
// orders 테이블에서 최근 100건을 가져와 응답한다.
// ============================================================
void AdminHandler::handleOrderMonitor(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 대기 주문 모니터링 (510)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        auto& db = MariaDBManager::getInstance();

        // 주문 목록을 조회한다 (고객명, 라이더명 포함).
        std::string q =
            "SELECT o.order_id, o.status, "
            "       u.login_id AS customer_name, "
            "       IFNULL(r.login_id, '') AS rider_name "
            "FROM orders o "
            "LEFT JOIN users u ON o.user_id = u.user_id "
            "LEFT JOIN users r ON o.rider_id = r.user_id "
            "ORDER BY o.order_id DESC "
            "LIMIT 100";

        DBResult rows = db.executeQuery(q);

        // 응답 JSON을 구성한다.
        json res;
        res["status"] = Status::SUCCESS;
        res["orders"] = json::array();

        for (auto& row : rows)
        {
            json item;
            item["order_id"]      = row.count("order_id")      ? row.at("order_id")      : "";
            item["status"]        = row.count("status")         ? row.at("status")         : "";
            item["customer_name"] = row.count("customer_name")  ? row.at("customer_name")  : "";
            item["rider_name"]    = row.count("rider_name")     ? row.at("rider_name")     : "";
            res["orders"].push_back(item);
        }

        sendResponse(session, CmdAdmin::REQ_MONITOR_ORDERS, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 주문 모니터링 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdAdmin::REQ_MONITOR_ORDERS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 511: handleRiderStatus — 라이더 현황을 조회한다.
// users 테이블에서 role='RIDER'인 사용자를 검색한다.
// ============================================================
void AdminHandler::handleRiderStatus(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 라이더 현황 조회 (511)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        auto& db = MariaDBManager::getInstance();

        // 라이더 목록을 조회한다.
        std::string q =
            "SELECT u.user_id, u.login_id AS rider_name, "
            "       IFNULL(u.status, 'OFFLINE') AS work_status "
            "FROM users u "
            "WHERE u.role = 'RIDER' "
            "ORDER BY u.user_id";

        DBResult rows = db.executeQuery(q);

        // 응답 JSON을 구성한다.
        json res;
        res["status"] = Status::SUCCESS;
        res["riders"] = json::array();

        for (auto& row : rows)
        {
            json item;
            item["user_id"]     = row.count("user_id")      ? row.at("user_id")     : "";
            item["rider_name"]  = row.count("rider_name")   ? row.at("rider_name")  : "";
            item["work_status"] = row.count("work_status")  ? row.at("work_status") : "";
            res["riders"].push_back(item);
        }

        sendResponse(session, CmdAdmin::REQ_RIDER_STATUS, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 라이더 현황 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdAdmin::REQ_RIDER_STATUS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 512: handleForceDispatch — 강제 배차를 수행한다.
// 지정된 주문에 라이더를 강제 배정한다.
// ============================================================
void AdminHandler::handleForceDispatch(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 강제 배차 (512)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        json req = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = req.value("rider_id", 0);

        // 파라미터 검증을 수행한다.
        if (orderId <= 0 || riderId <= 0) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 강제 배차 파라미터 부족 (order_id=" << orderId << ", rider_id=" << riderId << ")" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::BAD_REQUEST, "order_id, rider_id 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 주문에 라이더를 배정하고 상태를 변경한다.
        std::string q =
            "UPDATE orders SET rider_id = " + std::to_string(riderId) +
            ", status = 'DISPATCHED'"
            " WHERE order_id = " + std::to_string(orderId);

        if (!db.executeUpdate(q)) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 강제 배차 DB 실패 (order_id=" << orderId << ")" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::SERVER_ERROR, "DB 업데이트 실패");
            return;
        }

        // 성공 응답을 전송한다.
        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "강제 배차 완료";
        sendResponse(session, CmdAdmin::REQ_FORCE_DISPATCH, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 강제 배차 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 513: handleForceCancel — 배차를 강제 취소한다.
// 지정된 주문의 라이더를 해제하고 상태를 CANCELED로 변경한다.
// ============================================================
void AdminHandler::handleForceCancel(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 배차 강제 취소 (513)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        json req = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);

        // 파라미터 검증을 수행한다.
        if (orderId <= 0) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 배차 취소 파라미터 부족 (order_id=" << orderId << ")" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdAdmin::REQ_FORCE_CANCEL, Status::BAD_REQUEST, "order_id 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 라이더를 해제하고 주문 상태를 취소로 변경한다.
        std::string q =
            "UPDATE orders SET rider_id = NULL, status = 'CANCELED'"
            " WHERE order_id = " + std::to_string(orderId);

        if (!db.executeUpdate(q)) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "관리자" << std::endl;
            std::cout << "오류 : 배차 취소 DB 실패 (order_id=" << orderId << ")" << std::endl;
            std::cout << "-------------------------" << std::endl;
            sendError(session, CmdAdmin::REQ_FORCE_CANCEL, Status::SERVER_ERROR, "DB 업데이트 실패");
            return;
        }

        // 성공 응답을 전송한다.
        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "배차 취소 완료";
        sendResponse(session, CmdAdmin::REQ_FORCE_CANCEL, res);

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 배차 취소 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdAdmin::REQ_FORCE_CANCEL, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 520: handleManageReview — 리뷰를 관리한다.
// action에 따라 목록 조회 / 삭제 / 보이기·숨김 토글을 수행한다.
// ============================================================
void AdminHandler::handleManageReview(Session* session, const std::string& jsonBody)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "관리자" << std::endl;
    std::cout << "요청 : 리뷰 관리 (520)" << std::endl;
    std::cout << "-------------------------" << std::endl;

    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "list");
        auto& db = MariaDBManager::getInstance();

        // ── 리뷰 목록 조회 ──
        if (action == "list")
        {
            std::string q =
                "SELECT r.review_id, r.user_id, "
                "       IFNULL(u.login_id, '') AS user_name, "
                "       r.content, r.rating, "
                "       IFNULL(r.visible, 1) AS visible "
                "FROM reviews r "
                "LEFT JOIN users u ON r.user_id = u.user_id "
                "ORDER BY r.review_id DESC "
                "LIMIT 200";

            DBResult rows = db.executeQuery(q);

            json res;
            res["status"]  = Status::SUCCESS;
            res["reviews"] = json::array();

            for (auto& row : rows)
            {
                json item;
                item["review_id"] = row.count("review_id") ? std::stoi(row.at("review_id")) : 0;
                item["user_id"]   = row.count("user_name") ? row.at("user_name") : "";
                item["content"]   = row.count("content")   ? row.at("content")   : "";
                item["rating"]    = row.count("rating")     ? row.at("rating")    : "0";
                item["visible"]   = row.count("visible")    ? (row.at("visible") == "1") : true;
                res["reviews"].push_back(item);
            }

            sendResponse(session, CmdAdmin::REQ_MANAGE_REVIEW, res);
        }
        // ── 리뷰 삭제 ──
        else if (action == "delete")
        {
            int reviewId = req.value("review_id", 0);
            if (reviewId <= 0) {
                sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::BAD_REQUEST, "review_id 필요");
                return;
            }

            std::string q = "DELETE FROM reviews WHERE review_id = " + std::to_string(reviewId);
            if (!db.executeUpdate(q)) {
                sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::SERVER_ERROR, "삭제 실패");
                return;
            }

            json res;
            res["status"]  = Status::SUCCESS;
            res["message"] = "리뷰 삭제 완료";
            sendResponse(session, CmdAdmin::REQ_MANAGE_REVIEW, res);
        }
        // ── 보이기/숨김 토글 ──
        else if (action == "toggle_visibility")
        {
            int  reviewId = req.value("review_id", 0);
            bool visible  = req.value("visible", true);

            if (reviewId <= 0) {
                sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::BAD_REQUEST, "review_id 필요");
                return;
            }

            std::string q =
                "UPDATE reviews SET visible = " + std::string(visible ? "1" : "0") +
                " WHERE review_id = " + std::to_string(reviewId);

            if (!db.executeUpdate(q)) {
                sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::SERVER_ERROR, "업데이트 실패");
                return;
            }

            json res;
            res["status"]  = Status::SUCCESS;
            res["message"] = visible ? "리뷰 보이기" : "리뷰 숨김";
            sendResponse(session, CmdAdmin::REQ_MANAGE_REVIEW, res);
        }
        else
        {
            sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::BAD_REQUEST,
                      "알 수 없는 action: " + action);
        }

    } catch (const std::exception& e) {
        std::cout << "-------------------------" << std::endl;
        std::cout << "관리자" << std::endl;
        std::cout << "오류 : 리뷰 관리 예외 (" << e.what() << ")" << std::endl;
        std::cout << "-------------------------" << std::endl;
        sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::SERVER_ERROR, "서버 오류");
    }
}