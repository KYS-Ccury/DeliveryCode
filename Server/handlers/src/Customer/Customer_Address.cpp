// ================================================================
//  Customer_Address.cpp  — 주소 관련 핸들러 (신규)
//
//  프로토콜:
//    213  REQ_GET_ADDRESSES   — 주소 목록 조회
//    214  REQ_SAVE_ADDRESS    — 주소 추가
//    215  REQ_DELETE_ADDRESS  — 주소 삭제
//    216  REQ_DEFAULT_ADDRESS — 기본 주소 변경
//
//  DB 테이블: user_addresses
//    address_id  INT AUTO_INCREMENT PK
//    user_id     INT NOT NULL
//    address     VARCHAR(255) NOT NULL
//    label       VARCHAR(100) DEFAULT ''
//    is_default  TINYINT(1)   DEFAULT 0
//    created_at  DATETIME     DEFAULT NOW()
// ================================================================
#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include "CommonDB.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ================================================================
//  handleGetAddresses  (REQ_GET_ADDRESSES = 213)
//
//  요청: {}  (세션으로 userId 식별)
//  응답: {
//    "status": 2000,
//    "addresses": [
//      {"address_id":1, "address":"광주시 북구 용봉동",
//       "label":"집", "is_default":true},
//      ...
//    ]
//  }
// ================================================================
void CustomerHandler::handleGetAddresses(Session* session, const std::string& /*body*/)
{
    try {
        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_GET_ADDRESSES,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // user_addresses 테이블에서 해당 유저의 주소 목록 조회
        // is_default DESC → 기본 주소가 맨 앞에 오도록
        auto rows = db.executeQuery(
            "SELECT address_id, address, label, is_default "
            "FROM user_addresses "
            "WHERE user_id=" + std::to_string(userId) +
            " ORDER BY is_default DESC, address_id ASC");

        json addresses = json::array();
        for (const auto& r : rows) {
            json a;
            a["address_id"] = std::stoi(r.at("address_id"));
            a["address"]    = r.count("address") ? r.at("address") : "";
            a["label"]      = r.count("label")   ? r.at("label")   : "";
            a["is_default"] = (r.count("is_default") && r.at("is_default") == "1");
            addresses.push_back(a);
        }

        // ── user_addresses 가 비어있으면 users.address 를 기본 주소로 fallback ──
        // 회원가입 시 users.address 에만 저장되고 user_addresses 에는 없을 수 있음
        if (addresses.empty()) {
            auto uRows = db.executeQuery(
                "SELECT address FROM users "
                "WHERE user_id=" + std::to_string(userId) +
                "  AND address IS NOT NULL AND address != '' LIMIT 1");
            if (!uRows.empty()) {
                std::string userAddr = uRows[0].at("address");
                if (!userAddr.empty()) {
                    // user_addresses 에 자동 등록 (기본 주소로)
                    db.executeUpdate(
                        "INSERT INTO user_addresses "
                        "(user_id, address, label, is_default) "
                        "VALUES (" + std::to_string(userId) + ",'" +
                        CommonDB::getInstance().escape(userAddr) + "','기본주소',1)");

                    // 방금 INSERT 한 address_id 조회
                    auto newRows = db.executeQuery(
                        "SELECT address_id FROM user_addresses "
                        "WHERE user_id=" + std::to_string(userId) +
                        " ORDER BY address_id DESC LIMIT 1");

                    json a;
                    a["address_id"] = newRows.empty() ? 0
                                      : std::stoi(newRows[0].at("address_id"));
                    a["address"]    = userAddr;
                    a["label"]      = "기본주소";
                    a["is_default"] = true;
                    addresses.push_back(a);

                    std::cout << "[Customer] user_addresses 자동 생성: userId="
                              << userId << " addr=" << userAddr << "\n";
                }
            }
        }

        json res;
        res["status"]    = Status::SUCCESS;
        res["addresses"] = addresses;

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_GET_ADDRESSES, res.dump());

        std::cout << "[Customer] 주소목록 조회 userId=" << userId
                  << " count=" << rows.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleGetAddresses 예외: " << e.what() << "\n";
        sendError(session, CmdCustomer::REQ_GET_ADDRESSES,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleSaveAddress  (REQ_SAVE_ADDRESS = 214)
//
//  요청: { "address":"광주시 북구 용봉동",
//           "label":"집",
//           "is_default":true }
//  응답: { "status":2000, "address_id":N }
//
//  is_default==true 이면 기존 기본 주소를 먼저 해제 후 새 주소를 기본으로 설정
// ================================================================
void CustomerHandler::handleSaveAddress(Session* session, const std::string& body)
{
    try {
        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_SAVE_ADDRESS,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        json req = json::parse(body.empty() ? "{}" : body);
        std::string addr  = req.value("address", "");
        std::string label = req.value("label",   "");
        bool isDefault    = req.value("is_default", false);

        if (addr.empty()) {
            sendError(session, CmdCustomer::REQ_SAVE_ADDRESS,
                      Status::BAD_REQUEST, "주소를 입력해주세요.");
            return;
        }

        auto& db   = MariaDBManager::getInstance();
        auto& cdb  = CommonDB::getInstance();

        // 주소 개수 제한 (최대 10개)
        auto cntRows = db.executeQuery(
            "SELECT COUNT(*) AS cnt FROM user_addresses WHERE user_id=" +
            std::to_string(userId));
        if (!cntRows.empty()) {
            int cnt = 0;
            try { cnt = std::stoi(cntRows[0].at("cnt")); } catch (...) {}
            if (cnt >= 10) {
                sendError(session, CmdCustomer::REQ_SAVE_ADDRESS,
                          Status::BAD_REQUEST, "주소는 최대 10개까지 등록 가능합니다.");
                return;
            }
        }

        // is_default == true 이면 기존 기본 주소 해제
        if (isDefault) {
            db.executeUpdate(
                "UPDATE user_addresses SET is_default=0 "
                "WHERE user_id=" + std::to_string(userId));
        }

        // 새 주소 INSERT
        bool ok = db.executeUpdate(
            "INSERT INTO user_addresses (user_id, address, label, is_default) "
            "VALUES (" + std::to_string(userId) + ",'" +
            cdb.escape(addr) + "','" +
            cdb.escape(label) + "'," +
            (isDefault ? "1" : "0") + ")");

        if (!ok) {
            sendError(session, CmdCustomer::REQ_SAVE_ADDRESS,
                      Status::SERVER_ERROR, "주소 저장 실패");
            return;
        }

        // 방금 삽입된 address_id 조회
        auto idRows = db.executeQuery(
            "SELECT address_id FROM user_addresses "
            "WHERE user_id=" + std::to_string(userId) +
            " ORDER BY address_id DESC LIMIT 1");

        int newId = 0;
        if (!idRows.empty())
            try { newId = std::stoi(idRows[0].at("address_id")); } catch (...) {}

        json res;
        res["status"]     = Status::SUCCESS;
        res["address_id"] = newId;

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_SAVE_ADDRESS, res.dump());

        std::cout << "[Customer] 주소 추가 userId=" << userId
                  << " address_id=" << newId << "\n";

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleSaveAddress 예외: " << e.what() << "\n";
        sendError(session, CmdCustomer::REQ_SAVE_ADDRESS,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleDeleteAddress  (REQ_DELETE_ADDRESS = 215)
//
//  요청: { "address_id":N }
//  응답: { "status":2000 }
//
//  기본 주소를 삭제한 경우, 남은 주소 중 첫 번째를 기본으로 자동 설정
// ================================================================
void CustomerHandler::handleDeleteAddress(Session* session, const std::string& body)
{
    try {
        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_DELETE_ADDRESS,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        json req = json::parse(body.empty() ? "{}" : body);
        int addrId = req.value("address_id", 0);
        if (addrId <= 0) {
            sendError(session, CmdCustomer::REQ_DELETE_ADDRESS,
                      Status::BAD_REQUEST, "address_id가 유효하지 않습니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 삭제 대상이 기본 주소인지 확인
        auto chkRows = db.executeQuery(
            "SELECT is_default FROM user_addresses "
            "WHERE address_id=" + std::to_string(addrId) +
            " AND user_id=" + std::to_string(userId));

        bool wasDefault = false;
        if (!chkRows.empty())
            wasDefault = (chkRows[0].at("is_default") == "1");

        // 삭제 (본인 주소만)
        bool ok = db.executeUpdate(
            "DELETE FROM user_addresses "
            "WHERE address_id=" + std::to_string(addrId) +
            " AND user_id=" + std::to_string(userId));

        if (!ok) {
            sendError(session, CmdCustomer::REQ_DELETE_ADDRESS,
                      Status::SERVER_ERROR, "주소 삭제 실패");
            return;
        }

        // 기본 주소를 삭제했으면 남은 첫 번째 주소를 기본으로 설정
        if (wasDefault) {
            db.executeUpdate(
                "UPDATE user_addresses SET is_default=1 "
                "WHERE user_id=" + std::to_string(userId) +
                " ORDER BY address_id ASC LIMIT 1");
        }

        json res;
        res["status"] = Status::SUCCESS;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_DELETE_ADDRESS, res.dump());

        std::cout << "[Customer] 주소 삭제 userId=" << userId
                  << " address_id=" << addrId << "\n";

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleDeleteAddress 예외: " << e.what() << "\n";
        sendError(session, CmdCustomer::REQ_DELETE_ADDRESS,
                  Status::SERVER_ERROR, "서버 오류");
    }
}

// ================================================================
//  handleSetDefaultAddress  (REQ_DEFAULT_ADDRESS = 216)
//
//  요청: { "address_id":N }
//  응답: { "status":2000 }
// ================================================================
void CustomerHandler::handleSetDefaultAddress(Session* session, const std::string& body)
{
    try {
        int userId = getUserIdByFd(session->getFd());
        if (userId <= 0) {
            sendError(session, CmdCustomer::REQ_DEFAULT_ADDRESS,
                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
            return;
        }

        json req = json::parse(body.empty() ? "{}" : body);
        int addrId = req.value("address_id", 0);
        if (addrId <= 0) {
            sendError(session, CmdCustomer::REQ_DEFAULT_ADDRESS,
                      Status::BAD_REQUEST, "address_id가 유효하지 않습니다.");
            return;
        }

        auto& db = MariaDBManager::getInstance();

        // 기존 기본 주소 해제
        db.executeUpdate(
            "UPDATE user_addresses SET is_default=0 "
            "WHERE user_id=" + std::to_string(userId));

        // 새 기본 주소 설정 (본인 주소만)
        bool ok = db.executeUpdate(
            "UPDATE user_addresses SET is_default=1 "
            "WHERE address_id=" + std::to_string(addrId) +
            " AND user_id=" + std::to_string(userId));

        json res;
        res["status"] = ok ? Status::SUCCESS : Status::SERVER_ERROR;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_DEFAULT_ADDRESS, res.dump());

        std::cout << "[Customer] 기본 주소 변경 userId=" << userId
                  << " address_id=" << addrId << "\n";

    } catch (const std::exception& e) {
        std::cerr << "[Customer] handleSetDefaultAddress 예외: " << e.what() << "\n";
        sendError(session, CmdCustomer::REQ_DEFAULT_ADDRESS,
                  Status::SERVER_ERROR, "서버 오류");
    }
}
