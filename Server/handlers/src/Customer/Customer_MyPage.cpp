// ============================================================
//  Customer_MyPage.cpp
//
//  마이페이지 3개 핸들러:
//    209  REQ_MY_POINT        — 포인트 조회
//    211  REQ_CHANGE_PASSWORD — 비밀번호 변경
//    212  REQ_MY_INFO         — 내 정보 조회
// ============================================================
#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ============================================================
//  handleMyPoint  (REQ_MY_POINT = 209)
//
//  요청: {}
//  응답: { "status":2000, "points":N,
//           "history":[{"date":"...", "desc":"...", "amount":N}, ...] }
//
//  포인트 내역 = orders 테이블에서 DONE 상태 주문 (최근 20건)
//  적립 포인트 = total_price * 1% (소수점 버림)
// ============================================================
void CustomerHandler::handleMyPoint(Session* session, const std::string& /*body*/)
{
    try {
        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_MY_POINT,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // ── 1. 보유 포인트 ────────────────────────────────────
        auto ptRows = db.executeQuery(
            "SELECT COALESCE(point,0) AS point "
            "FROM customer_profiles WHERE user_id=" + std::to_string(userId));

        int point = 0;
        if (!ptRows.empty() && ptRows[0].count("point") && !ptRows[0].at("point").empty())
            try { point = std::stoi(ptRows[0].at("point")); } catch (...) {}

        // ── 2. 포인트 내역: 완료된 주문에서 생성 (최근 20건) ─
        // 적립: DONE 주문 → total_price * 1%
        // 사용: total_price > actual_paid 인 경우는 별도 컬럼 없으므로
        //        현재는 적립 내역만 표시
        auto histRows = db.executeQuery(
            "SELECT DATE_FORMAT(created_at, '%Y-%m-%d') AS d, "
            "       r.restaurant_name AS store_name, "
            "       total_price "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.customer_id=" + std::to_string(userId) +
            "  AND o.status='DONE' "
            "ORDER BY o.created_at DESC LIMIT 20");

        json history = json::array();
        for (const auto& row : histRows) {
            int price = 0;
            if (row.count("total_price") && !row.at("total_price").empty())
                try { price = std::stoi(row.at("total_price")); } catch (...) {}

            int earned = price / 100;  // 1% 적립
            if (earned <= 0) continue;

            json item;
            item["date"]   = row.count("d")          ? row.at("d")          : "";
            std::string storeName = row.count("store_name") && !row.at("store_name").empty() ? row.at("store_name") : "주문";
            item["desc"]   = storeName + " 포인트 적립";
            item["amount"] = earned;
            history.push_back(item);
        }

        json res;
        res["status"]  = Status::SUCCESS;
        res["points"]  = point;
        res["history"] = history;

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_MY_POINT, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleMyPoint 예외: " << e.what() << std::endl;
        sendError(session, CmdCustomer::REQ_MY_POINT,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
//  handleChangePassword  (REQ_CHANGE_PASSWORD = 211)
//
//  요청: { "current_pw":"...", "new_pw":"..." }
//  응답: { "status":2000 } or { "status":4001, "message":"..." }
// ============================================================
void CustomerHandler::handleChangePassword(Session* session,
                                            const std::string& body)
{
    try {
        json req = json::parse(body.empty() ? "{}" : body);
        std::string curPw = req.value("current_pw", "");
        std::string newPw = req.value("new_pw",     "");

        if (curPw.empty() || newPw.empty()) {
            sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                      Status::BAD_REQUEST, "현재/새 비밀번호를 입력해주세요.");
            return;
        }
        if (newPw.size() < 6) {
            sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                      Status::BAD_REQUEST, "비밀번호는 6자 이상이어야 합니다.");
            return;
        }

        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 현재 비밀번호 확인 (평문 비교 — 프로젝트 수준)
        auto rows = db.executeQuery(
            "SELECT password FROM users WHERE user_id=" + std::to_string(userId));

        if (rows.empty()) {
            sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                      Status::NOT_FOUND, "사용자 정보를 찾을 수 없습니다.");
            return;
        }

        std::string storedPw = rows[0].at("password");
        if (storedPw != curPw) {
            sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                      Status::UNAUTHORIZED, "현재 비밀번호가 올바르지 않습니다.");
            return;
        }

        // 비밀번호 변경
        // SQL 이스케이프 (간단 버전)
        auto esc = [](const std::string& s) {
            std::string o; for (char c : s) { if (c=='\'' || c=='\\') o+='\\'; o+=c; } return o;
        };

        bool ok = db.executeUpdate(
            "UPDATE users SET password='" + esc(newPw) + "' "
            "WHERE user_id=" + std::to_string(userId));

        if (!ok) {
            sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                      Status::SERVER_ERROR, "비밀번호 변경 실패");
            return;
        }

        json res;
        res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_CHANGE_PASSWORD, res.dump());

        std::cout << "[Customer] 비밀번호 변경 userId=" << userId << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleChangePassword 예외: " << e.what() << std::endl;
        sendError(session, CmdCustomer::REQ_CHANGE_PASSWORD,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ============================================================
//  handleMyInfo  (REQ_MY_INFO = 212)
//
//  요청: {}  (로그인 세션으로 userId 식별)
//  응답: { "status":2000, "login_id":"...", "name":"...",
//          "phone":"...", "address":"...", "grade":"일반회원" }
// ============================================================
void CustomerHandler::handleMyInfo(Session* session, const std::string& /*body*/)
{
    try {
        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_MY_INFO,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        auto rows = db.executeQuery(
            "SELECT u.login_id, u.name, u.phone, "
            "       COALESCE(u.address,'') AS address, "
            "       COALESCE(cp.point,0)  AS point "
            "FROM users u "
            "LEFT JOIN customer_profiles cp ON cp.user_id=u.user_id "
            "WHERE u.user_id=" + std::to_string(userId));

        if (rows.empty()) {
            sendError(session, CmdCustomer::REQ_MY_INFO,
                      Status::NOT_FOUND, "사용자 정보를 찾을 수 없습니다.");
            return;
        }

        const auto& r = rows[0];
        int point = 0;
        if (r.count("point") && !r.at("point").empty())
            try { point = std::stoi(r.at("point")); } catch (...) {}

        // 등급 계산 (포인트 기준)
        std::string grade;
        if      (point >= 10000) grade = "VIP";
        else if (point >= 5000)  grade = "골드";
        else if (point >= 1000)  grade = "실버";
        else                     grade = "일반회원";

        json res;
        res["status"]   = Status::SUCCESS;
        res["login_id"] = r.count("login_id") ? r.at("login_id") : "";
        res["name"]     = r.count("name")     ? r.at("name")     : "";
        res["phone"]    = r.count("phone")    ? r.at("phone")    : "";
        res["address"]  = r.count("address")  ? r.at("address")  : "";
        res["grade"]    = grade;
        res["point"]    = point;

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_MY_INFO, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleMyInfo 예외: " << e.what() << std::endl;
        sendError(session, CmdCustomer::REQ_MY_INFO,
                  Status::SERVER_ERROR, "서버 오류");
    }
}
