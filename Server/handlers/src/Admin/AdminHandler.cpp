#include "AdminHandler.h"
#include "ChatHandler.h"
#include "RiderHandler.h"
#include "Session.h"
#include "Packet.h"          // CmdAdmin, ClientType, Status 포함
#include "MariaDBManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include "Types.h" 

using json = nlohmann::json;


// ─── 관리자 세션 등록/해제 ────────────────────────────────
void AdminHandler::registerSession(int fd, int adminId) {
    ChatHandler::registerAdmin(fd, adminId);
}
void AdminHandler::unregisterSession(int fd) {
    ChatHandler::unregisterAdmin(fd);
}

void AdminHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    std::cout << "[AdminHandler] 관리자 명령어 수신: " << protocol << std::endl;

    switch (protocol) {
        case CmdAdmin::REQ_MONITOR_ORDERS: // 또는 통계 관련 프로토콜
            handleGetStats(session, jsonBody);
            break;

        case CmdAdmin::REQ_MANAGE_REVIEW: // 또는 사용자 관리 프로토콜
            handleBanUser(session, jsonBody);
            break;

        case CmdAdmin::REQ_FORCE_CANCEL:
            handleForceCancel(session, jsonBody);
            break;

        // 채팅 (관리자가 보내는 600~602)
        case CmdChat::REQ_CREATE_ROOM:
        case CmdChat::REQ_SEND_MSG:
        case CmdChat::REQ_GET_MSGS:
            ChatHandler::process(session, protocol, jsonBody, ClientType::ADMIN);
            break;

        // 관리자 로그인 (101) - 세션 등록
        case CmdCommon::REQ_LOGIN:
            handleAdminLogin(session, jsonBody);
            break;

        case CmdCommon::REQ_LOGOUT:
            ChatHandler::unregisterAdmin(session->getFd());
            break;

        default:
            std::cerr << "[Admin] 알 수 없는 관리자 프로토콜: " << protocol << std::endl;
            break;
    }
}





//////////////////////////////참고만 하길 바람..../////////////////
// ---------------------------------------------------------
// [기능 구현] 1. 서버 통계 조회 (기능 개발 중)
// ---------------------------------------------------------
void AdminHandler::handleGetStats(Session* session, const std::string& jsonBody) {
    auto& db = MariaDBManager::getInstance();
    
    // 비즈니스 로직: 통계 쿼리 실행 예정
    json res;
    res["status"] = Status::SUCCESS;
    res["today_orders"] = 1503;
    res["active_riders"] = 124;

    session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), 
                        CmdAdmin::REQ_MONITOR_ORDERS, res.dump());
}

// ---------------------------------------------------------
// [기능 구현] 2. 사용자 계정 정지 (기능 개발 중)
// ---------------------------------------------------------
void AdminHandler::handleBanUser(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        auto& db = MariaDBManager::getInstance();

        // 비즈니스 로직: UPDATE users SET is_banned = 1 ... 예정
        std::string target = req.value("target_user_id", "Unknown");

        json res;
        res["status"] = Status::SUCCESS;
        res["message"] = target + " 계정 정지 완료";

        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN), 
                            CmdAdmin::REQ_MANAGE_REVIEW, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleBanUser] JSON Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------
// [기능 구현] 3. 강제 주문 취소 (기능 개발 중)
// ---------------------------------------------------------
void AdminHandler::handleForceCancel(Session* session, const std::string& jsonBody) {
    // 관리자 권한 강제 취소 로직 구현 예정
    std::cout << "[Admin] 강제 취소 로직 실행 예정" << std::endl;
}



//////////// 마리아 DB 참고 예시 ////////////////////////
// #include "MariaDBManager.h"

// // 1. SELECT 예시 (고객 정보 조회)
// auto& db = MariaDBManager::getInstance();
// std::string query = "SELECT name, phone FROM users WHERE login_id = 'test_user'";
// DBResult rows = db.executeQuery(query);

// if (!rows.empty()) {
//     std::string userName = rows[0]["name"];
//     std::string userPhone = rows[0]["phone"];
// }

// // 2. UPDATE 예시 (사장님이 주문을 수락하여 상태 변경)
// std::string updateQuery = "UPDATE orders SET status = 'ACCEPTED' WHERE order_id = 123";
// bool success = db.executeUpdate(updateQuery);

// // 3. INSERT 예시 (새로운 주문 생성 후 PK 받아오기)
// std::string insertQuery = "INSERT INTO orders (customer_id, restaurant_id, total_price) VALUES (1, 10, 15000)";
// if (db.executeUpdate(insertQuery)) {
//     uint64_t newOrderId = db.getLastInsertId(); // 생성된 order_id (예: 124) 획득
// }
// ─────────────────────────────────────────────────────────
//  101: 관리자 로그인 + ChatHandler 세션 등록
// ─────────────────────────────────────────────────────────
void AdminHandler::handleAdminLogin(Session* session, const std::string& jsonBody) {
    try {
        json req = jsonBody.empty() ? json::object() : json::parse(jsonBody);
        std::string loginId  = req.value("login_id", "");
        std::string password = req.value("password", "");

        if (loginId.empty() || password.empty()) {
            json res; res["status"] = Status::BAD_REQUEST;
            res["message"] = "아이디/비밀번호를 입력하세요.";
            session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN),
                                CmdCommon::REQ_LOGIN, res.dump());
            return;
        }

        auto& db = MariaDBManager::getInstance();
        auto esc = [](const std::string& s) {
            std::string o;
            for (char c : s) { if (c=='\'' || c=='\\' || c=='"') o+='\\'; o+=c; }
            return o;
        };

        DBResult rows = db.executeQuery(
            "SELECT user_id, name FROM users "
            "WHERE login_id = '" + esc(loginId) + "' "
            "  AND password = '" + esc(password) + "' "
            "  AND role = 'ADMIN' AND status = 'ACTIVE' LIMIT 1");

        if (rows.empty()) {
            json res; res["status"] = Status::UNAUTHORIZED;
            res["message"] = "아이디 또는 비밀번호가 올바르지 않습니다.";
            session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN),
                                CmdCommon::REQ_LOGIN, res.dump());
            return;
        }

        int adminId = std::stoi(rows[0].at("user_id"));
        // ChatHandler에 관리자 세션 등록 (채팅 Push 수신용)
        ChatHandler::registerAdmin(session->getFd(), adminId);

        json res;
        res["status"]   = Status::SUCCESS;
        res["admin_id"] = adminId;
        res["name"]     = rows[0].at("name");
        session->sendPacket(static_cast<uint8_t>(ClientType::ADMIN),
                            CmdCommon::REQ_LOGIN, res.dump());

        std::cout << "[Admin] 관리자 로그인: " << rows[0].at("name")
                  << " (id=" << adminId << ")" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleAdminLogin] 예외: " << e.what() << std::endl;
    }
}
