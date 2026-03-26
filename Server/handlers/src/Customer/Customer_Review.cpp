#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"

#include "CommonDB.h"

using json = nlohmann::json;
void CustomerHandler::handleWriteReview(Session* session, const std::string& body) {
    try {
        auto& db  = MariaDBManager::getInstance();
        json  req = json::parse(body);

        int         uid     = getUserIdByFd(session->getFd());
        int         orderID = req.value("order_id", 0);
        int         rating  = req.value("rating",   5);
        std::string content = req.value("content",  "");

        // ★ 1. uid 검증 수정 및 필수값 체크
        if (uid <= 0 || orderID <= 0 || content.empty()) {
            sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "필수 항목 누락 또는 비로그인");
            return;
        }

        // ★ 2. 별점(Rating) 어뷰징 방지 (1 ~ 5 사이로 강제 고정)
        if (rating < 1) rating = 1;
        if (rating > 5) rating = 5;

        // 3. 내 주문인지 & 배달 완료(DONE) 상태인지 확인
        auto chk = db.executeQuery("SELECT order_id FROM orders WHERE order_id=" + std::to_string(orderID) + " AND customer_id=" + std::to_string(uid) + " AND status='DONE'");
        if (chk.empty()) { sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::FORBIDDEN, "배달 완료된 본인 주문만 리뷰 가능"); return; }

        // 4. 리뷰 중복 작성 방지
        auto dup = db.executeQuery("SELECT review_id FROM reviews WHERE order_id=" + std::to_string(orderID));
        if (!dup.empty()) { sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::BAD_REQUEST, "이미 리뷰를 작성했습니다"); return; }

        // 5. 리뷰 저장
        bool ok = db.executeUpdate("INSERT INTO reviews (order_id, customer_id, rating, content) VALUES ("
            + std::to_string(orderID) + "," + std::to_string(uid) + "," + std::to_string(rating) + ",'" + CommonDB::getInstance().escape(content) + "')");
        if (!ok) { sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::SERVER_ERROR, "저장 실패"); return; }

        // 6. 식당 평균 별점 업데이트 로직 (훌륭합니다!)
        db.executeUpdate(
            "UPDATE restaurants r SET r.rating_avg = ("
            "SELECT AVG(rv.rating) FROM reviews rv JOIN orders o ON o.order_id=rv.order_id "
            "WHERE o.restaurant_id=r.restaurant_id) WHERE r.restaurant_id=("
            "SELECT restaurant_id FROM orders WHERE order_id=" + std::to_string(orderID) + ")");

        json res; res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_WRITE_REVIEW, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_WRITE_REVIEW, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleReviewList(Session* session, const std::string& body) {
    try {
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        int   storeID = req.value("store_id", 0);

        // ★ 가벼운 파라미터 방어 로직 추가
        if (storeID <= 0) {
            sendError(session, CmdCustomer::REQ_REVIEW_LIST, Status::BAD_REQUEST, "잘못된 식당 ID입니다.");
            return;
        }

        auto rows = db.executeQuery(
            "SELECT rv.review_id, u.name AS author, rv.rating, rv.content, rv.owner_reply, rv.replied_at "
            "FROM reviews rv JOIN orders o ON o.order_id = rv.order_id JOIN users u ON u.user_id = rv.customer_id "
            "WHERE o.restaurant_id=" + std::to_string(storeID) + " ORDER BY rv.review_id DESC LIMIT 30");

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
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_REVIEW_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_REVIEW_LIST, Status::SERVER_ERROR, e.what());
    }
}