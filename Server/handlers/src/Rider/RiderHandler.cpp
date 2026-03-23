// ============================================================
//  RiderHandler.cpp
//  라이더 클라이언트의 모든 요청을 처리하는 핸들러
//
//  패킷 형식 (서버 수신):
//    PacketHeader { clientType=3, protocol, bodyLength } + JSON Body
//
//  패킷 형식 (서버 송신):
//    sendPacket(clientType=3, protocol, json.dump())
//
//  주요 프로토콜:
//    101 : 로그인       (공통, CmdCommon::REQ_LOGIN)
//    102 : 로그아웃     (공통)
//    400 : 배차 리스트 조회
//    401 : 배차 수락
//    402 : 배차 거절
//    403 : 픽업 완료
//    404 : 배달 완료
//    405 : 내 배달 내역
//    406 : 출퇴근/배차수락 상태 변경
//    407 : GPS 위치 전송
// ============================================================
#include "RiderHandler.h"
#include "EpollServer.h"
#include "Session.h"
#include "Packet.h"
#include "MariaDBManager.h"
#include "Types.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <mutex>

using json = nlohmann::json;

// ─── 정적 멤버 초기화 ─────────────────────────────────────
std::unordered_map<int, int> RiderHandler::s_fdToRider;
std::unordered_map<int, int> RiderHandler::s_riderToFd;
std::mutex                   RiderHandler::s_sessionMtx;

// ─────────────────────────────────────────────────────────
//  세션 등록 / 해제 / 조회
// ─────────────────────────────────────────────────────────
void RiderHandler::registerSession(int fd, int riderId) {
    std::lock_guard<std::mutex> lk(s_sessionMtx);
    s_fdToRider[fd]      = riderId;
    s_riderToFd[riderId] = fd;
}

void RiderHandler::unregisterSession(int fd) {
    std::lock_guard<std::mutex> lk(s_sessionMtx);
    auto it = s_fdToRider.find(fd);
    if (it != s_fdToRider.end()) {
        s_riderToFd.erase(it->second);
        s_fdToRider.erase(it);
    }
}

int RiderHandler::getRiderIdByFd(int fd) {
    std::lock_guard<std::mutex> lk(s_sessionMtx);
    auto it = s_fdToRider.find(fd);
    return (it != s_fdToRider.end()) ? it->second : 0;
}

// ─────────────────────────────────────────────────────────
//  메인 디스패치
// ─────────────────────────────────────────────────────────
void RiderHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    switch (protocol) {
        // 공통 인증
        case CmdCommon::REQ_SIGNUP:      handleSignup    (session, jsonBody); break;
        case CmdCommon::REQ_LOGIN:       handleLogin     (session, jsonBody); break;
        case CmdCommon::REQ_LOGOUT:      handleLogout    (session, jsonBody); break;
        case CmdCommon::REQ_GET_PROFILE: handleGetProfile(session, jsonBody); break;

        // 라이더 전용
        case CmdRider::REQ_DISPATCH_LIST:   handleDispatchList  (session, jsonBody); break;
        case CmdRider::REQ_ACCEPT_DISPATCH: handleAcceptDispatch(session, jsonBody); break;
        case CmdRider::REQ_REJECT_DISPATCH: handleRejectDispatch(session, jsonBody); break;
        case CmdRider::REQ_PICKUP_DONE:     handlePickupDone    (session, jsonBody); break;
        case CmdRider::REQ_DELIVERY_DONE:   handleDeliveryDone  (session, jsonBody); break;
        case CmdRider::REQ_MY_DISPATCHES:   handleMyDispatches  (session, jsonBody); break;
        case CmdRider::REQ_WORK_STATUS:     handleWorkStatus    (session, jsonBody); break;
        case CmdRider::REQ_SEND_GPS:        handleUpdateGps     (session, jsonBody); break;

        default:
            std::cerr << "[Rider] 알 수 없는 프로토콜: " << protocol << std::endl;
            break;
    }
}

// ─────────────────────────────────────────────────────────
//  헬퍼: 에러 응답 전송
// ─────────────────────────────────────────────────────────
static void sendError(Session* session, uint16_t protocol,
                      uint16_t statusCode, const std::string& message) {
    json res;
    res["status"]  = statusCode;
    res["message"] = message;
    session->sendPacket(static_cast<uint8_t>(ClientType::RIDER), protocol, res.dump());
}

// ─────────────────────────────────────────────────────────
//  헬퍼: SQL 인젝션 방지용 문자열 이스케이프
// ─────────────────────────────────────────────────────────
static std::string escapeStr(const std::string& s) {
    // 간이 이스케이프 (실제 프로덕션은 mysql_real_escape_string 사용 권장)
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// ─────────────────────────────────────────────────────────
//  101: 로그인
//  요청  JSON: { "login_id": "...", "password": "..." }
//  응답  JSON: { "status":2000, "rider_id":1, "name":"...",
//               "vehicle_type":"...", "is_working":false }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleLogin(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string loginId  = req.value("login_id", "");
        std::string password = req.value("password", "");

        if (loginId.empty() || password.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::BAD_REQUEST, "아이디/비밀번호를 입력하세요.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // ① users 테이블에서 인증 (role = RIDER 만 허용)
        std::string q =
            "SELECT u.user_id, u.name, u.phone, u.address, u.status, "
            "       rp.vehicle_type, rp.is_working, rp.is_accepting "
            "FROM users u "
            "JOIN rider_profiles rp ON rp.user_id = u.user_id "
            "WHERE u.login_id = '" + escapeStr(loginId) + "' "
            "  AND u.password = '" + escapeStr(password) + "' "
            "  AND u.role = 'RIDER' "
            "  AND u.status = 'ACTIVE' "
            "LIMIT 1";

        DBResult rows = db.executeQuery(q);
        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_LOGIN, Status::UNAUTHORIZED,
                      "아이디 또는 비밀번호가 올바르지 않습니다.");
            return;
        }

        const DBRow& row    = rows[0];
        int    riderId      = std::stoi(row.at("user_id"));
        std::string name    = row.at("name");
        std::string phone   = row.at("phone");
        std::string vehicle = row.at("vehicle_type");
        bool isWorking      = (row.at("is_working") == "1");
        bool isAccepting    = (row.at("is_accepting") == "1");

        // ② rider_profiles.is_online = TRUE 갱신
        db.executeUpdate(
            "UPDATE rider_profiles SET is_online = TRUE "
            "WHERE user_id = " + std::to_string(riderId));

        // ③ 세션 테이블에 등록 (fd ↔ riderId)
        registerSession(session->getFd(), riderId);

        json res;
        res["status"]       = Status::SUCCESS;
        res["rider_id"]     = riderId;
        res["name"]         = name;
        res["phone"]        = phone;
        res["vehicle_type"] = vehicle;
        res["is_working"]   = isWorking;
        res["is_accepting"] = isAccepting;

        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdCommon::REQ_LOGIN, res.dump());

        std::cout << "[Rider] 로그인 성공: " << name << " (id=" << riderId << ")" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleLogin] 예외: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_LOGIN, Status::SERVER_ERROR, "서버 오류가 발생했습니다.");
    }
}

// ─────────────────────────────────────────────────────────
//  102: 로그아웃
//  요청  JSON: { "rider_id": 1 }
//  응답  JSON: { "status":2000 }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleLogout(Session* session, const std::string& jsonBody) {
    try {
        int riderId = getRiderIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdCommon::REQ_LOGOUT, Status::UNAUTHORIZED, "로그인 상태가 아닙니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        db.executeUpdate(
            "UPDATE rider_profiles "
            "SET is_online = FALSE, is_accepting = FALSE, is_working = FALSE "
            "WHERE user_id = " + std::to_string(riderId));

        unregisterSession(session->getFd());

        json res;
        res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdCommon::REQ_LOGOUT, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleLogout] 예외: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────
//  400: 배차 리스트 조회
//  요청  JSON: {} (rider_id는 세션에서 가져옴)
//  응답  JSON: { "status":2000,
//               "orders": [
//                 { "order_id":1, "store_name":"...",
//                   "pickup_addr":"...", "dest_addr":"...",
//                   "delivery_fee":3000, "total_price":15000,
//                   "elapsed_sec":130 },
//                 ...
//               ] }
//
//  WAITING_PICKUP 상태이며 rider_id가 NULL인 주문 목록 반환
// ─────────────────────────────────────────────────────────
void RiderHandler::handleDispatchList(Session* session, const std::string& jsonBody) {
    try {
        auto& db = MariaDBManager::getInstance();

        // WAITING_PICKUP 상태이고 라이더 미배정인 주문 조회
        std::string q =
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       r.address AS pickup_addr, o.delivery_address AS dest_addr, "
            "       r.base_delivery_fee AS delivery_fee, o.total_price, "
            "       TIMESTAMPDIFF(SECOND, o.created_at, NOW()) AS elapsed_sec "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.status = 'WAITING_PICKUP' "
            "  AND o.rider_id IS NULL "
            "  AND o.delivery_method = '배달' "
            "ORDER BY o.created_at ASC";

        DBResult rows = db.executeQuery(q);

        json res;
        res["status"] = Status::SUCCESS;
        res["orders"] = json::array();

        for (const auto& row : rows) {
            json item;
            item["order_id"]     = std::stoi(row.at("order_id"));
            item["store_name"]   = row.at("store_name");
            item["pickup_addr"]  = row.at("pickup_addr");
            item["dest_addr"]    = row.at("dest_addr");
            item["delivery_fee"] = std::stoi(row.at("delivery_fee"));
            item["total_price"]  = std::stoi(row.at("total_price"));
            item["elapsed_sec"]  = std::stoi(row.at("elapsed_sec"));
            res["orders"].push_back(item);
        }

        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_DISPATCH_LIST, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleDispatchList] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_DISPATCH_LIST, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  401: 배차 수락
//  요청  JSON: { "order_id": 1 }
//  응답  JSON: { "status":2000, "order_id":1,
//               "order_code":"ORD0001", "store_name":"...",
//               "pickup_addr":"...", "dest_addr":"...",
//               "customer_request":"...", "delivery_fee":3000 }
//
//  orders.rider_id 를 현재 라이더로 업데이트 (경쟁 조건 방지: mutex 필요)
// ─────────────────────────────────────────────────────────
void RiderHandler::handleAcceptDispatch(Session* session, const std::string& jsonBody) {
    try {
        json req  = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        if (orderId <= 0) {
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH, Status::BAD_REQUEST, "order_id 누락");
            return;
        }

        int riderId = getRiderIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // ★ 배달 완료 뮤텍스: orders 동시 수정 방지 (규칙.txt 명시)
        // MariaDBManager 내부 db_mutex 가 executeUpdate 단위로 보호됨
        // 여기서는 SELECT → UPDATE 를 하나의 트랜잭션으로 처리

        // ① 트랜잭션 시작
        db.executeUpdate("START TRANSACTION");

        // ② 해당 주문이 아직 미배정 상태인지 확인 (FOR UPDATE 로 row-lock)
        std::string checkQ =
            "SELECT order_id FROM orders "
            "WHERE order_id = " + std::to_string(orderId) +
            "  AND status = 'WAITING_PICKUP' "
            "  AND rider_id IS NULL "
            "FOR UPDATE";

        DBResult check = db.executeQuery(checkQ);
        if (check.empty()) {
            db.executeUpdate("ROLLBACK");
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH, Status::NOT_FOUND,
                      "이미 다른 라이더가 배차를 수락했거나 유효하지 않은 주문입니다.");
            return;
        }

        // ③ orders.rider_id 업데이트, 상태 DELIVERING으로 변경
        bool ok = db.executeUpdate(
            "UPDATE orders "
            "SET rider_id = " + std::to_string(riderId) + ", "
            "    status = 'DELIVERING' "
            "WHERE order_id = " + std::to_string(orderId));

        if (!ok) {
            db.executeUpdate("ROLLBACK");
            sendError(session, CmdRider::REQ_ACCEPT_DISPATCH, Status::SERVER_ERROR, "DB 업데이트 실패");
            return;
        }

        // ④ dispatch_logs 기록
        db.executeUpdate(
            "INSERT INTO dispatch_logs (order_id, rider_id, result) "
            "VALUES (" + std::to_string(orderId) + ", " +
                         std::to_string(riderId) + ", 'ACCEPT')");

        // ⑤ order_status_logs 기록
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) "
            "VALUES (" + std::to_string(orderId) + ", 'WAITING_PICKUP', 'DELIVERING', " +
                         std::to_string(riderId) + ")");

        db.executeUpdate("COMMIT");

        // ⑥ 주문 상세 조회 (클라이언트에게 전달할 정보)
        std::string detailQ =
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       r.address AS pickup_addr, o.delivery_address AS dest_addr, "
            "       r.phone AS store_phone, r.base_delivery_fee AS delivery_fee, "
            "       o.total_price "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id = " + std::to_string(orderId);

        DBResult detail = db.executeQuery(detailQ);

        json res;
        res["status"]   = Status::SUCCESS;
        res["order_id"] = orderId;

        if (!detail.empty()) {
            const DBRow& d = detail[0];
            // 주문 코드: ORD + 0-padded order_id
            std::ostringstream code;
            code << "ORD" << std::setw(6) << std::setfill('0') << orderId;
            res["order_code"]    = code.str();
            res["store_name"]    = d.at("store_name");
            res["pickup_addr"]   = d.at("pickup_addr");
            res["store_phone"]   = d.at("store_phone");
            res["dest_addr"]     = d.at("dest_addr");
            res["delivery_fee"]  = std::stoi(d.at("delivery_fee"));
            res["total_price"]   = std::stoi(d.at("total_price"));
        }

        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_ACCEPT_DISPATCH, res.dump());

        std::cout << "[Rider] 배차 수락: orderId=" << orderId
                  << " riderId=" << riderId << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleAcceptDispatch] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_ACCEPT_DISPATCH, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  402: 배차 거절
//  요청  JSON: { "order_id": 1, "reason": "MANUAL" }
//  응답  JSON: { "status":2000 }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleRejectDispatch(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        std::string reason = req.value("reason", "MANUAL"); // MANUAL or TIMEOUT

        int riderId = getRiderIdByFd(session->getFd());
        if (riderId <= 0 || orderId <= 0) {
            sendError(session, CmdRider::REQ_REJECT_DISPATCH, Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // dispatch_logs 에 REJECT 기록
        db.executeUpdate(
            "INSERT INTO dispatch_logs (order_id, rider_id, result) "
            "VALUES (" + std::to_string(orderId) + ", " +
                         std::to_string(riderId) + ", '" + escapeStr(reason) + "')");

        json res;
        res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_REJECT_DISPATCH, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleRejectDispatch] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_REJECT_DISPATCH, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  403: 픽업 완료
//  요청  JSON: { "order_id": 1 }
//  응답  JSON: { "status":2000, "message":"픽업 완료, 배달을 시작합니다." }
// ─────────────────────────────────────────────────────────
void RiderHandler::handlePickupDone(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = getRiderIdByFd(session->getFd());

        if (orderId <= 0 || riderId <= 0) {
            sendError(session, CmdRider::REQ_PICKUP_DONE, Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // orders.status = 'DELIVERING' (픽업 완료 = 배달 시작)
        // 단, 이미 해당 라이더의 주문인지 확인
        bool ok = db.executeUpdate(
            "UPDATE orders SET status = 'DELIVERING' "
            "WHERE order_id = " + std::to_string(orderId) +
            "  AND rider_id = " + std::to_string(riderId) +
            "  AND status IN ('DELIVERING')");
        // 참고: ACCEPT 시 이미 DELIVERING으로 바꿨으므로
        // 픽업 완료는 클라이언트 UI 단계 확인용으로만 사용됨
        // 필요시 별도 상태(PICKUP_DONE 등) 추가 가능

        // order_status_logs 기록
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) "
            "VALUES (" + std::to_string(orderId) + ", 'DELIVERING', 'DELIVERING', " +
                         std::to_string(riderId) + ")");

        json res;
        res["status"]  = Status::SUCCESS;
        res["message"] = "픽업 완료, 배달을 시작합니다.";
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_PICKUP_DONE, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handlePickupDone] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_PICKUP_DONE, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  404: 배달 완료
//  요청  JSON: { "order_id": 1 }
//  응답  JSON: { "status":2000, "delivery_fee":3000 }
//
//  ★ 규칙.txt: "배달 완료를 찍으면 DB 수정할 때 뮤텍스를 걸어야함 꼭"
//     → MariaDBManager 내부 db_mutex + 트랜잭션으로 처리
// ─────────────────────────────────────────────────────────
void RiderHandler::handleDeliveryDone(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        int orderId = req.value("order_id", 0);
        int riderId = getRiderIdByFd(session->getFd());

        if (orderId <= 0 || riderId <= 0) {
            sendError(session, CmdRider::REQ_DELIVERY_DONE, Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // ★ 트랜잭션 + DB내부 뮤텍스 = 이중 보호
        db.executeUpdate("START TRANSACTION");

        // ① 주문 상태 확인 (FOR UPDATE = row-lock)
        DBResult check = db.executeQuery(
            "SELECT o.order_id, r.base_delivery_fee AS delivery_fee "
            "FROM orders o "
            "JOIN restaurants r ON r.restaurant_id = o.restaurant_id "
            "WHERE o.order_id = " + std::to_string(orderId) +
            "  AND o.rider_id = " + std::to_string(riderId) +
            "  AND o.status = 'DELIVERING' "
            "FOR UPDATE");

        if (check.empty()) {
            db.executeUpdate("ROLLBACK");
            sendError(session, CmdRider::REQ_DELIVERY_DONE, Status::NOT_FOUND,
                      "유효하지 않은 주문이거나 이미 완료된 주문입니다.");
            return;
        }

        int deliveryFee = std::stoi(check[0].at("delivery_fee"));

        // ② orders.status = 'DONE'
        db.executeUpdate(
            "UPDATE orders SET status = 'DONE' "
            "WHERE order_id = " + std::to_string(orderId));

        // ③ rider_earnings 기록
        db.executeUpdate(
            "INSERT INTO rider_earnings (rider_id, order_id, delivery_fee) "
            "VALUES (" + std::to_string(riderId) + ", " +
                         std::to_string(orderId) + ", " +
                         std::to_string(deliveryFee) + ")");

        // ④ order_status_logs 기록
        db.executeUpdate(
            "INSERT INTO order_status_logs (order_id, from_status, to_status, changed_by) "
            "VALUES (" + std::to_string(orderId) + ", 'DELIVERING', 'DONE', " +
                         std::to_string(riderId) + ")");

        // ⑤ 개인정보 보호: 3시간 후 블라인드 처리 (규칙.txt)
        // orders.is_masked = TRUE 는 스케줄러나 별도 배치로 처리해야 하나,
        // 여기서는 배달 완료 즉시 마스킹 처리 (is_masked=TRUE)
        db.executeUpdate(
            "UPDATE orders SET is_masked = TRUE "
            "WHERE order_id = " + std::to_string(orderId));

        db.executeUpdate("COMMIT");

        json res;
        res["status"]       = Status::SUCCESS;
        res["delivery_fee"] = deliveryFee;
        res["message"]      = "배달이 완료되었습니다. 수고하셨습니다!";

        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_DELIVERY_DONE, res.dump());

        std::cout << "[Rider] 배달 완료: orderId=" << orderId
                  << " fee=" << deliveryFee << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[handleDeliveryDone] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_DELIVERY_DONE, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  405: 내 배달 내역 (이전 내역)
//  요청  JSON: {} (rider_id 세션 자동)
//  응답  JSON: { "status":2000,
//               "records": [
//                 { "order_id":1, "order_code":"ORD000001",
//                   "store_name":"...", "delivery_fee":3000,
//                   "status":"DONE", "created_at":"2025-03-18 19:35:00" },
//                 ...
//               ] }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleMyDispatches(Session* session, const std::string& jsonBody) {
    try {
        int riderId = getRiderIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdRider::REQ_MY_DISPATCHES, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        json req = jsonBody.empty() ? json{} : json::parse(jsonBody);
        bool summaryOnly = req.value("summary_only", false);
        auto& db = MariaDBManager::getInstance();

        // ── 오늘 요약 모드 (MyPageDlg 에서 호출) ──────────
        if (summaryOnly) {
            DBResult rows = db.executeQuery(
                "SELECT COUNT(*) AS cnt, IFNULL(SUM(re.delivery_fee),0) AS total "
                "FROM orders o "
                "JOIN rider_earnings re ON re.order_id = o.order_id "
                "WHERE o.rider_id = " + std::to_string(riderId) +
                "  AND DATE(o.created_at) = CURDATE() "
                "  AND o.status = 'DONE'");

            json res;
            res["status"]      = Status::SUCCESS;
            res["today_count"] = rows.empty() ? 0 : std::stoi(rows[0].at("cnt"));
            res["today_fee"]   = rows.empty() ? 0 : std::stoi(rows[0].at("total"));
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdRider::REQ_MY_DISPATCHES, res.dump());
            return;
        }

        // ── 전체 이전 내역 ────────────────────────────────

        std::string q =
            "SELECT o.order_id, r.restaurant_name AS store_name, "
            "       o.status, o.total_price, "
            "       re.delivery_fee, "
            "       DATE_FORMAT(o.created_at, '%m/%d %H:%i') AS created_at "
            "FROM orders o "
            "JOIN restaurants r  ON r.restaurant_id = o.restaurant_id "
            "LEFT JOIN rider_earnings re ON re.order_id = o.order_id "
            "                           AND re.rider_id = " + std::to_string(riderId) +
            " WHERE o.rider_id = " + std::to_string(riderId) +
            "   AND o.status = 'DONE' "
            "ORDER BY o.created_at DESC "
            "LIMIT 50";

        DBResult rows = db.executeQuery(q);

        json res;
        res["status"]  = Status::SUCCESS;
        res["records"] = json::array();

        for (const auto& row : rows) {
            json item;
            int oid = std::stoi(row.at("order_id"));
            std::ostringstream code;
            code << "ORD" << std::setw(6) << std::setfill('0') << oid;

            item["order_id"]     = oid;
            item["order_code"]   = code.str();
            item["store_name"]   = row.at("store_name");
            item["delivery_fee"] = row.at("delivery_fee").empty() ? 0
                                   : std::stoi(row.at("delivery_fee"));
            item["status"]       = row.at("status");
            item["created_at"]   = row.at("created_at");
            res["records"].push_back(item);
        }

        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_MY_DISPATCHES, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleMyDispatches] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_MY_DISPATCHES, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  406: 출퇴근 / 배차수락 상태 변경
//  요청  JSON: { "action": "ONLINE"|"OFFLINE"|"DISPATCH_ON"|"DISPATCH_OFF" }
//  응답  JSON: { "status":2000, "action":"..." }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleWorkStatus(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "");
        int riderId        = getRiderIdByFd(session->getFd());

        if (riderId <= 0 || action.empty()) {
            sendError(session, CmdRider::REQ_WORK_STATUS, Status::BAD_REQUEST, "파라미터 오류");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        std::string updateQ;

        if (action == "ONLINE") {
            updateQ = "UPDATE rider_profiles SET is_working = TRUE, is_accepting = TRUE "
                      "WHERE user_id = " + std::to_string(riderId);
        } else if (action == "OFFLINE") {
            updateQ = "UPDATE rider_profiles SET is_working = FALSE, is_accepting = FALSE "
                      "WHERE user_id = " + std::to_string(riderId);
        } else if (action == "DISPATCH_ON") {
            updateQ = "UPDATE rider_profiles SET is_accepting = TRUE "
                      "WHERE user_id = " + std::to_string(riderId);
        } else if (action == "DISPATCH_OFF") {
            updateQ = "UPDATE rider_profiles SET is_accepting = FALSE "
                      "WHERE user_id = " + std::to_string(riderId);
        } else {
            sendError(session, CmdRider::REQ_WORK_STATUS, Status::BAD_REQUEST,
                      "알 수 없는 action: " + action);
            return;
        }

        db.executeUpdate(updateQ);

        json res;
        res["status"] = Status::SUCCESS;
        res["action"] = action;
        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdRider::REQ_WORK_STATUS, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleWorkStatus] 예외: " << e.what() << std::endl;
        sendError(session, CmdRider::REQ_WORK_STATUS, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  407: GPS 위치 전송 (응답 없음, 최대한 가볍게)
//  요청  JSON: { "latitude": 35.1234, "longitude": 126.5678 }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleUpdateGps(Session* session, const std::string& jsonBody) {
    try {
        json req    = json::parse(jsonBody);
        double lat  = req.value("latitude",  0.0);
        double lng  = req.value("longitude", 0.0);
        int riderId = getRiderIdByFd(session->getFd());

        if (riderId <= 0 || (lat == 0.0 && lng == 0.0)) return;

        auto& db = MariaDBManager::getInstance();

        // users 테이블 위경도 + rider_profiles last_location_at 갱신
        std::ostringstream q;
        q << "UPDATE users "
          << "SET latitude = " << lat << ", longitude = " << lng
          << " WHERE user_id = " << riderId;
        db.executeUpdate(q.str());

        db.executeUpdate(
            "UPDATE rider_profiles SET last_location_at = NOW() "
            "WHERE user_id = " + std::to_string(riderId));

        // GPS는 응답 패킷 없음 (규칙.txt: 단발성 통신)

    } catch (const std::exception& e) {
        // GPS는 에러 로그만 (클라이언트에 응답 불필요)
        std::cerr << "[handleUpdateGps] 예외: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────
//  Push: 관리자/시스템이 라이더에게 신규 배차 알림 전송
//  NTF_NEW_DISPATCH (408)
// ─────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────
//  100: 회원가입 (라이더 전용)
//  요청 JSON:
//    action == "CHECK_ID" : { "action":"CHECK_ID", "login_id":"..." }
//    action == "REGISTER" : { "action":"REGISTER", "login_id","password",
//                              "name","phone","address","vehicle_type","role":"RIDER" }
//
//  CHECK_ID 응답: { "status":2000, "available":true/false }
//  REGISTER 응답: { "status":2000 } 또는 { "status":4000, "message":"..." }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleSignup(Session* session, const std::string& jsonBody) {
    try {
        json req = json::parse(jsonBody);
        std::string action = req.value("action", "");
        auto& db = MariaDBManager::getInstance();

        // ── 아이디 중복 확인 ──────────────────────────────
        if (action == "CHECK_ID") {
            std::string loginId = req.value("login_id", "");
            if (loginId.empty()) {
                sendError(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "login_id 누락");
                return;
            }
            DBResult rows = db.executeQuery(
                "SELECT user_id FROM users WHERE login_id = '" + escapeStr(loginId) + "' LIMIT 1");

            json res;
            res["status"]    = Status::SUCCESS;
            res["available"] = rows.empty(); // 없으면 true (사용 가능)
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdCommon::REQ_SIGNUP, res.dump());
            return;
        }

        // ── 회원가입 ──────────────────────────────────────
        if (action == "REGISTER") {
            std::string loginId     = req.value("login_id",     "");
            std::string password    = req.value("password",     "");
            std::string name        = req.value("name",         "");
            std::string phone       = req.value("phone",        "");
            std::string address     = req.value("address",      "");
            std::string vehicleType = req.value("vehicle_type", "");

            if (loginId.empty() || password.empty() || name.empty() || phone.empty()) {
                sendError(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST, "필수 항목 누락");
                return;
            }

            // 중복 최종 확인
            DBResult dup = db.executeQuery(
                "SELECT user_id FROM users WHERE login_id = '" + escapeStr(loginId) + "' LIMIT 1");
            if (!dup.empty()) {
                sendError(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST,
                          "이미 사용 중인 아이디입니다.");
                return;
            }

            // ① users 테이블 INSERT
            bool ok = db.executeUpdate(
                "INSERT INTO users (login_id, password, role, name, phone, address, status) "
                "VALUES ('"  + escapeStr(loginId)  + "', '"
                             + escapeStr(password)  + "', 'RIDER', '"
                             + escapeStr(name)      + "', '"
                             + escapeStr(phone)     + "', '"
                             + escapeStr(address)   + "', 'ACTIVE')");

            if (!ok) {
                sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "가입 처리 실패");
                return;
            }

            uint64_t userId = db.getLastInsertId();

            // ② rider_profiles INSERT
            // 오토바이/자동차는 is_admin_blocked=TRUE (운전면허 확인 전까지 대기)
            bool needLicense = (vehicleType == "오토바이" || vehicleType == "자동차");
            db.executeUpdate(
                "INSERT INTO rider_profiles (user_id, vehicle_type, is_working, is_online, "
                "                            is_accepting, is_admin_blocked) "
                "VALUES (" + std::to_string(userId) + ", '"
                           + escapeStr(vehicleType) + "', FALSE, FALSE, FALSE, "
                           + (needLicense ? "TRUE" : "FALSE") + ")");

            json res;
            res["status"]  = Status::SUCCESS;
            res["message"] = needLicense
                ? "가입 완료! 운전면허 확인 후 배달을 시작할 수 있습니다."
                : "가입이 완료되었습니다. 로그인 후 배달을 시작하세요.";

            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdCommon::REQ_SIGNUP, res.dump());

            std::cout << "[Rider] 회원가입 완료: " << loginId
                      << " (userId=" << userId << ")" << std::endl;
            return;
        }

        sendError(session, CmdCommon::REQ_SIGNUP, Status::BAD_REQUEST,
                  "알 수 없는 action: " + action);

    } catch (const std::exception& e) {
        std::cerr << "[handleSignup] 예외: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_SIGNUP, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
//  104: 프로필 조회 / 비밀번호·계좌 변경
//  요청 JSON action 분기:
//    {} (빈 body)     → 내 정보 조회
//    { "action":"CHANGE_PW",   "cur_pw","new_pw" }
//    { "action":"CHANGE_ACCT", "bank","holder","account" }
//    { "action":"VEHICLE",     "vehicle_type":"..." }
// ─────────────────────────────────────────────────────────
void RiderHandler::handleGetProfile(Session* session, const std::string& jsonBody) {
    try {
        int riderId = getRiderIdByFd(session->getFd());
        if (riderId <= 0) {
            sendError(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        json req = jsonBody.empty() ? json{} : json::parse(jsonBody);
        std::string action = req.value("action", "");
        auto& db = MariaDBManager::getInstance();

        // ── 비밀번호 변경 ─────────────────────────────────
        if (action == "CHANGE_PW") {
            std::string curPw = req.value("cur_pw", "");
            std::string newPw = req.value("new_pw", "");
            if (curPw.empty() || newPw.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE, Status::BAD_REQUEST, "파라미터 누락");
                return;
            }
            // 현재 비밀번호 확인
            DBResult chk = db.executeQuery(
                "SELECT user_id FROM users "
                "WHERE user_id = " + std::to_string(riderId) +
                "  AND password = '" + escapeStr(curPw) + "'");
            if (chk.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED,
                          "현재 비밀번호가 올바르지 않습니다.");
                return;
            }
            db.executeUpdate(
                "UPDATE users SET password = '" + escapeStr(newPw) + "' "
                "WHERE user_id = " + std::to_string(riderId));

            json res; res["status"] = Status::SUCCESS; res["action"] = "CHANGE_PW";
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 계좌 변경 (users 테이블에 별도 컬럼 없으므로 address 필드 활용 or
        //               실제 프로덕션은 payment_methods 테이블 사용) ─────────
        if (action == "CHANGE_ACCT") {
            // 현재 스키마는 payment_methods 테이블에 계좌 정보 저장
            std::string bank    = req.value("bank",    "");
            std::string holder  = req.value("holder",  "");
            std::string account = req.value("account", "");
            if (bank.empty() || holder.empty() || account.empty()) {
                sendError(session, CmdCommon::REQ_GET_PROFILE, Status::BAD_REQUEST, "파라미터 누락");
                return;
            }
            // 기존 계좌 삭제 후 재입력 (단순 구현)
            db.executeUpdate(
                "DELETE FROM payment_methods WHERE user_id = " + std::to_string(riderId) +
                "  AND method_type = 'BANK'");
            // 마스킹된 계좌번호: 뒤 4자리만 저장
            std::string masked = std::string(account.size() > 4 ? account.size() - 4 : 0, '*')
                                 + account.substr(account.size() > 4 ? account.size() - 4 : 0);
            db.executeUpdate(
                "INSERT INTO payment_methods (user_id, method_type, card_alias, "
                "                             card_num_masked, is_default) "
                "VALUES (" + std::to_string(riderId) + ", 'BANK', '"
                           + escapeStr(bank + " " + holder) + "', '"
                           + escapeStr(masked) + "', TRUE)");

            json res; res["status"] = Status::SUCCESS; res["action"] = "CHANGE_ACCT";
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 배달수단 변경 ─────────────────────────────────
        if (action == "VEHICLE") {
            std::string vehicleType = req.value("vehicle_type", "");
            if (!vehicleType.empty()) {
                db.executeUpdate(
                    "UPDATE rider_profiles SET vehicle_type = '" + escapeStr(vehicleType) + "' "
                    "WHERE user_id = " + std::to_string(riderId));
            }
            json res; res["status"] = Status::SUCCESS; res["action"] = "VEHICLE";
            session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                                CmdCommon::REQ_GET_PROFILE, res.dump());
            return;
        }

        // ── 기본: 내 정보 조회 ────────────────────────────
        DBResult rows = db.executeQuery(
            "SELECT u.user_id, u.login_id, u.name, u.phone, u.address, "
            "       rp.vehicle_type, rp.is_working, rp.is_accepting, "
            "       pm.card_alias AS bank_info, pm.card_num_masked AS acct_masked "
            "FROM users u "
            "JOIN rider_profiles rp ON rp.user_id = u.user_id "
            "LEFT JOIN payment_methods pm ON pm.user_id = u.user_id AND pm.method_type='BANK' "
            "WHERE u.user_id = " + std::to_string(riderId));

        if (rows.empty()) {
            sendError(session, CmdCommon::REQ_GET_PROFILE, Status::NOT_FOUND, "사용자 없음");
            return;
        }
        const DBRow& r = rows[0];
        json res;
        res["status"]       = Status::SUCCESS;
        res["rider_id"]     = riderId;
        res["login_id"]     = r.at("login_id");
        res["name"]         = r.at("name");
        res["phone"]        = r.at("phone");
        res["address"]      = r.at("address");
        res["vehicle_type"] = r.at("vehicle_type");
        res["is_working"]   = (r.at("is_working") == "1");
        res["is_accepting"] = (r.at("is_accepting") == "1");
        res["bank_info"]    = r.count("bank_info")   ? r.at("bank_info")   : "";
        res["acct_masked"]  = r.count("acct_masked") ? r.at("acct_masked") : "";

        session->sendPacket(static_cast<uint8_t>(ClientType::RIDER),
                            CmdCommon::REQ_GET_PROFILE, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[handleGetProfile] 예외: " << e.what() << std::endl;
        sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, "서버 오류");
    }
}

// ─────────────────────────────────────────────────────────
bool RiderHandler::pushDispatch(int riderFd, int orderId,
                                const std::string& storeName,
                                const std::string& pickupAddr,
                                const std::string& destAddr,
                                int deliveryFee)
{
    if (!EpollServer::s_instance) {
        std::cerr << "[pushDispatch] EpollServer 인스턴스 없음" << std::endl;
        return false;
    }

    auto session = EpollServer::s_instance->getSession(riderFd);
    if (!session) {
        std::cerr << "[pushDispatch] riderFd=" << riderFd << " 세션 없음 (접속 끊김?)" << std::endl;
        return false;
    }

    json push;
    push["order_id"]     = orderId;
    push["store_name"]   = storeName;
    push["pickup_addr"]  = pickupAddr;
    push["dest_addr"]    = destAddr;
    push["delivery_fee"] = deliveryFee;

    bool ok = session->sendPacket(
        static_cast<uint8_t>(ClientType::RIDER),
        CmdRider::NTF_NEW_DISPATCH,   // 408
        push.dump()
    );

    std::cout << "[Rider Push] orderId=" << orderId
              << " riderFd=" << riderFd
              << (ok ? " → 전송 성공" : " → 전송 실패") << std::endl;
    return ok;
}
