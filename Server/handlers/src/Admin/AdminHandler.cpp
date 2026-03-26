#include "AdminHandler.h"
#include "MariaDBManager.h"
#include "EpollServer.h"
#include <iostream>

using json = nlohmann::json;

// ============================================================
// 싱글톤
// ============================================================
AdminHandler::AdminHandler()
    : BaseHandler(ClientType::ADMIN, "ADMIN")
{
}

AdminHandler& AdminHandler::getInstance()
{
    static AdminHandler instance;
    return instance;
}

// ============================================================
// 로그인 성공 훅 (BaseHandler::handleLogin에서 호출)
// ============================================================
void AdminHandler::onLoginSuccess(Session* session, int userId, const json& req)
{
    std::cout << "[Admin] 로그인 성공 — userId=" << userId
              << ", fd=" << session->getFd() << std::endl;

    session->setUserID(userId);
    session->setUserType(static_cast<uint8_t>(ClientType::ADMIN));

    json res;
    res["status"]  = Status::SUCCESS;
    res["message"] = "관리자 로그인 성공";
    res["user_id"] = userId;
    sendResponse(session, CmdCommon::REQ_LOGIN, res);
}

// ============================================================
// 로그아웃 훅
// ============================================================
void AdminHandler::onLogout(Session* session, int userId)
{
    std::cout << "[Admin] 로그아웃 — userId=" << userId << std::endl;

    json res;
    res["status"]  = Status::SUCCESS;
    res["message"] = "로그아웃 완료";
    sendResponse(session, CmdCommon::REQ_LOGOUT, res);
}

// ============================================================
// 패킷 분기 (Dispatcher에서 호출)
// ============================================================
void AdminHandler::handlePacket(Session* session, uint16_t protocol, const std::string& jsonBody)
{
    switch (protocol)
    {
    // ── 500번대 ──
    case CmdAdmin::REQ_MONITOR_ORDERS: handleOrderMonitor (session, jsonBody); break;
    case CmdAdmin::REQ_RIDER_STATUS:   handleRiderStatus  (session, jsonBody); break;
    case CmdAdmin::REQ_FORCE_DISPATCH: handleForceDispatch(session, jsonBody); break;
    case CmdAdmin::REQ_FORCE_CANCEL:   handleForceCancel  (session, jsonBody); break;
    case CmdAdmin::REQ_MANAGE_REVIEW:  handleManageReview (session, jsonBody); break;

    // ── 600번대 채팅 ──
    case CmdChat::REQ_SEND_MSG:   handleSendMsg (session, jsonBody); break;
    case CmdChat::REQ_GET_MSGS:   handleGetMsgs (session, jsonBody); break;
    case CmdChat::REQ_ROOM_LIST:  handleRoomList(session, jsonBody); break;

    default:
        std::cout << "[Admin] 미처리 프로토콜: " << protocol << std::endl;
        sendError(session, protocol, Status::BAD_REQUEST, "알 수 없는 요청");
        break;
    }
}

// ============================================================
// 510: 대기 주문 모니터링
// ============================================================
void AdminHandler::handleOrderMonitor(Session* session, const std::string& jsonBody)
{
    try {
        auto& db = MariaDBManager::getInstance();

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
        std::cerr << "[Admin] 주문 모니터링 오류: " << e.what() << std::endl;
        sendError(session, CmdAdmin::REQ_MONITOR_ORDERS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 511: 라이더 현황
// ============================================================
void AdminHandler::handleRiderStatus(Session* session, const std::string& jsonBody)
{
    try {
        auto& db = MariaDBManager::getInstance();

        std::string q =
            "SELECT u.user_id, u.login_id AS rider_name, "
            "       IFNULL(u.status, 'OFFLINE') AS work_status "
            "FROM users u "
            "WHERE u.role = 'RIDER' "
            "ORDER BY u.user_id";

        DBResult rows = db.executeQuery(q);

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
        std::cerr << "[Admin] 라이더 현황 오류: " << e.what() << std::endl;
        sendError(session, CmdAdmin::REQ_RIDER_STATUS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 512: 강제 배차
// ============================================================
void AdminHandler::handleForceDispatch(Session* session, const std::string& jsonBody)
{
    try {
        json req = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = req.value("rider_id", 0);

        if (orderId <= 0 || riderId <= 0) {
            sendError(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::BAD_REQUEST,
                      "order_id, rider_id 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        std::string q =
            "UPDATE orders SET rider_id = " + std::to_string(riderId) +
            ", status = 'DISPATCHED'"
            " WHERE order_id = " + std::to_string(orderId);

        if (!db.executeUpdate(q)) {
            sendError(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::SERVER_ERROR, "DB 업데이트 실패");
            return;
        }

        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "강제 배차 완료";
        sendResponse(session, CmdAdmin::REQ_FORCE_DISPATCH, res);

    } catch (const std::exception& e) {
        std::cerr << "[Admin] 강제 배차 오류: " << e.what() << std::endl;
        sendError(session, CmdAdmin::REQ_FORCE_DISPATCH, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 513: 배차 강제 취소
// ============================================================
void AdminHandler::handleForceCancel(Session* session, const std::string& jsonBody)
{
    try {
        json req = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);

        if (orderId <= 0) {
            sendError(session, CmdAdmin::REQ_FORCE_CANCEL, Status::BAD_REQUEST, "order_id 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        std::string q =
            "UPDATE orders SET rider_id = NULL, status = 'CANCELED'"
            " WHERE order_id = " + std::to_string(orderId);

        if (!db.executeUpdate(q)) {
            sendError(session, CmdAdmin::REQ_FORCE_CANCEL, Status::SERVER_ERROR, "DB 업데이트 실패");
            return;
        }

        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "배차 취소 완료";
        sendResponse(session, CmdAdmin::REQ_FORCE_CANCEL, res);

    } catch (const std::exception& e) {
        std::cerr << "[Admin] 배차 취소 오류: " << e.what() << std::endl;
        sendError(session, CmdAdmin::REQ_FORCE_CANCEL, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 520: 리뷰 관리 (list / delete / toggle_visibility)
// ============================================================
void AdminHandler::handleManageReview(Session* session, const std::string& jsonBody)
{
    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "list");

        auto& db = MariaDBManager::getInstance();

        // ── list ──
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
        // ── delete ──
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
        // ── toggle_visibility ──
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
        std::cerr << "[Admin] 리뷰 관리 오류: " << e.what() << std::endl;
        sendError(session, CmdAdmin::REQ_MANAGE_REVIEW, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 601: 메시지 전송
// ============================================================
void AdminHandler::handleSendMsg(Session* session, const std::string& jsonBody)
{
    try {
        json req = json::parse(jsonBody);
        std::string roomId  = req.value("room_id", "");
        std::string message = req.value("message", "");

        if (roomId.empty() || message.empty()) {
            sendError(session, CmdChat::REQ_SEND_MSG, Status::BAD_REQUEST, "room_id, message 필요");
            return;
        }

        int adminUserId = getUserIdByFd(session->getFd());
        auto& db = MariaDBManager::getInstance();

        std::string q =
            "INSERT INTO chat_messages (room_id, sender_id, message, is_admin, created_at) "
            "VALUES ('" + escapeStr(roomId) + "', " + std::to_string(adminUserId) +
            ", '" + escapeStr(message) + "', 1, NOW())";

        if (!db.executeUpdate(q)) {
            sendError(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "메시지 저장 실패");
            return;
        }

        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "전송 완료";
        sendResponse(session, CmdChat::REQ_SEND_MSG, res);

    } catch (const std::exception& e) {
        std::cerr << "[Admin] 메시지 전송 오류: " << e.what() << std::endl;
        sendError(session, CmdChat::REQ_SEND_MSG, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 602: 메시지 조회
// ============================================================
void AdminHandler::handleGetMsgs(Session* session, const std::string& jsonBody)
{
    try {
        json req = json::parse(jsonBody);
        std::string roomId = req.value("room_id", "");

        if (roomId.empty()) {
            sendError(session, CmdChat::REQ_GET_MSGS, Status::BAD_REQUEST, "room_id 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        std::string q =
            "SELECT message, is_admin, created_at "
            "FROM chat_messages "
            "WHERE room_id = '" + escapeStr(roomId) + "' "
            "ORDER BY created_at ASC "
            "LIMIT 500";

        DBResult rows = db.executeQuery(q);

        json res;
        res["status"]   = Status::SUCCESS;
        res["messages"] = json::array();

        for (auto& row : rows)
        {
            json item;
            item["text"]     = row.count("message")    ? row.at("message")    : "";
            item["is_admin"] = row.count("is_admin")   ? (row.at("is_admin") == "1") : false;
            item["time"]     = row.count("created_at") ? row.at("created_at") : "";
            res["messages"].push_back(item);
        }

        sendResponse(session, CmdChat::REQ_GET_MSGS, res);

    } catch (const std::exception& e) {
        std::cerr << "[Admin] 메시지 조회 오류: " << e.what() << std::endl;
        sendError(session, CmdChat::REQ_GET_MSGS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
// 603: 채팅방 목록
// ============================================================
void AdminHandler::handleRoomList(Session* session, const std::string& jsonBody)
{
    try {
        auto& db = MariaDBManager::getInstance();

        std::string q =
            "SELECT cr.room_id, "
            "       IFNULL(u.login_id, '') AS user_name, "
            "       IFNULL(u.role, '고객') AS role, "
            "       ( SELECT cm.message FROM chat_messages cm "
            "         WHERE cm.room_id = cr.room_id "
            "         ORDER BY cm.created_at DESC LIMIT 1 "
            "       ) AS last_message "
            "FROM chat_rooms cr "
            "LEFT JOIN users u ON cr.user_id = u.user_id "
            "ORDER BY cr.room_id DESC "
            "LIMIT 100";

        DBResult rows = db.executeQuery(q);

        json res;
        res["status"] = Status::SUCCESS;
        res["rooms"]  = json::array();

        for (auto& row : rows)
        {
            json item;
            item["room_id"]      = row.count("room_id")      ? row.at("room_id")      : "";
            item["user_name"]    = row.count("user_name")     ? row.at("user_name")     : "";
            item["role"]         = row.count("role")          ? row.at("role")           : "고객";
            item["last_message"] = row.count("last_message")  ? row.at("last_message")  : "";
            res["rooms"].push_back(item);
        }

        sendResponse(session, CmdChat::REQ_ROOM_LIST, res);

    } catch (const std::exception& e) {
        std::cerr << "[Admin] 채팅방 목록 오류: " << e.what() << std::endl;
        sendError(session, CmdChat::REQ_ROOM_LIST, Status::SERVER_ERROR, "서버 오류");
    }
}