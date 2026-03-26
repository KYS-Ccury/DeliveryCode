#include "CommonDB.h"
#include <iostream>
#include "Protocol.h"

// ── 내부 유틸 ──────────────────────────────────────────────
//  [로그인]

int CommonDB::queryLogin(const std::string& loginId,
                         const std::string& password,
                         const std::string& role) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE login_id='"  + MariaDBManager::escape(loginId)  + "' "
        "  AND password='"  + MariaDBManager::escape(password) + "' "
        "  AND role='"      + MariaDBManager::escape(role)      + "' "
        "  AND status='ACTIVE' LIMIT 1");

    if (rows.empty()) return -1;
    try { return std::stoi(rows[0].at("user_id")); }
    catch (...) { return -1; }
}

//  [회원가입]
bool CommonDB::queryCheckDuplicateId(const std::string& loginId) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE login_id='" + MariaDBManager::escape(loginId) + "' LIMIT 1");
    return rows.empty();   // true = 사용 가능 (중복 없음)
}

int CommonDB::queryInsertUser(const std::string& loginId,
                              const std::string& password,
                              const std::string& role,
                              const std::string& name,
                              const std::string& phone,
                              const std::string& address) {
    auto& db = MariaDBManager::getInstance();
    bool ok = db.executeUpdate(
        "INSERT INTO users (login_id, password, role, name, phone, address, status) "
        "VALUES ('"
        + MariaDBManager::escape(loginId)  + "','"
        + MariaDBManager::escape(password) + "','"
        + MariaDBManager::escape(role)     + "','"
        + MariaDBManager::escape(name)     + "','"
        + MariaDBManager::escape(phone)    + "','"
        + MariaDBManager::escape(address)  + "',"
        "'ACTIVE')");

    if (!ok) return -1;
    try { return static_cast<int>(db.getLastInsertId()); }
    catch (...) { return -1; }
}

//  [기본 정보 조회]
CommonDB::UserBasic CommonDB::queryUserBasic(int userId) {
    UserBasic result;
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT name, phone, address FROM users "
        "WHERE user_id=" + std::to_string(userId) + " LIMIT 1");

    if (rows.empty()) return result;
    result.found   = true;
    result.name    = rows[0].count("name")    ? rows[0].at("name")    : "";
    result.phone   = rows[0].count("phone")   ? rows[0].at("phone")   : "";
    result.address = rows[0].count("address") ? rows[0].at("address") : "";
    return result;
}

//  [비밀번호]
bool CommonDB::queryCheckPassword(int userId, const std::string& password) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE user_id=" + std::to_string(userId) +
        "  AND password='" + MariaDBManager::escape(password) + "' LIMIT 1");
    return !rows.empty();
}

bool CommonDB::queryChangePassword(int userId, const std::string& newPassword) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE users SET password='" + MariaDBManager::escape(newPassword) +
        "' WHERE user_id=" + std::to_string(userId));
}

//  [결제수단]
std::vector<CommonDB::PaymentMethod> CommonDB::queryPaymentMethods(int userId) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT payment_method_id AS id, card_alias AS alias, "
        "       card_num_masked AS masked, method_type, is_default "
        "FROM payment_methods "
        "WHERE user_id=" + std::to_string(userId) +
        " ORDER BY is_default DESC, payment_method_id ASC");

    std::vector<PaymentMethod> result;
    for (const auto& r : rows) {
        PaymentMethod pm;
        pm.id         = r.count("id")          && !r.at("id").empty()
                        ? std::stoi(r.at("id")) : 0;
        pm.alias      = r.count("alias")       ? r.at("alias")       : "";
        pm.maskedNum  = r.count("masked")      ? r.at("masked")      : "";
        pm.methodType = r.count("method_type") ? r.at("method_type") : "CARD";
        pm.isDefault  = r.count("is_default") && r.at("is_default") == "1";
        result.push_back(pm);
    }
    return result;
}

bool CommonDB::queryAddCard(int userId,
                            const std::string& alias,
                            const std::string& maskedNum,
                            const std::string& methodType) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "INSERT INTO payment_methods "
        "(user_id, method_type, card_alias, card_num_masked, is_default) "
        "VALUES (" + std::to_string(userId) + ",'"
        + MariaDBManager::escape(methodType) + "','"
        + MariaDBManager::escape(alias)      + "','"
        + MariaDBManager::escape(maskedNum)  + "',0)");
}

bool CommonDB::queryDeleteCard(int userId, int paymentMethodId) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "DELETE FROM payment_methods "
        "WHERE payment_method_id=" + std::to_string(paymentMethodId) +
        "  AND user_id=" + std::to_string(userId));
}

bool CommonDB::querySetDefaultCard(int userId, int paymentMethodId) {
    auto& db = MariaDBManager::getInstance();
    // 기존 기본 카드 전부 해제
    db.executeUpdate(
        "UPDATE payment_methods SET is_default=0 "
        "WHERE user_id=" + std::to_string(userId));
    // 새 기본 카드 설정
    return db.executeUpdate(
        "UPDATE payment_methods SET is_default=1 "
        "WHERE payment_method_id=" + std::to_string(paymentMethodId) +
        "  AND user_id=" + std::to_string(userId));
}

//  [계정 상태]

bool CommonDB::querySetUserStatus(int userId, const std::string& status) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE users SET status='" + MariaDBManager::escape(status) +
        "' WHERE user_id=" + std::to_string(userId));
}

// ============================================================
//  [MiddleHandler 라우팅 구현부]
// ============================================================
nlohmann::json CommonDB::process(uint16_t dbProtocol, const nlohmann::json& reqJson) {
    if (dbProtocol == CmdDBCommon::REQ_DB_SIGNUP)      return signup(reqJson);
    if (dbProtocol == CmdDBCommon::REQ_DB_LOGIN)       return login(reqJson);
    // 프로필 정보 조회/카드 처리 등의 로직을 getProfile 함수로 만들어 연결하시면 됩니다.
    // if (dbProtocol == CmdDBCommon::REQ_DB_GET_PROFILE) return getProfile(reqJson);

    return {{"status", Status::SERVER_ERROR}, {"message", "Unknown CommonDB Protocol"}};
}

nlohmann::json CommonDB::signup(const nlohmann::json& reqBody) {
    nlohmann::json res;
    std::string action = reqBody.value("action", "");
    std::string loginId = reqBody.value("id", "");

    // 1. 중복 아이디 체크
    if (action == "CHECK_ID") {
        res["status"] = Status::SUCCESS;
        res["available"] = getInstance().queryCheckDuplicateId(loginId);
        return res;
    }

    // 2. 실제 회원가입 로직
    if (!getInstance().queryCheckDuplicateId(loginId)) {
        res["status"] = Status::BAD_REQUEST;
        res["message"] = "이미 사용 중인 아이디입니다.";
        return res;
    }

    std::string pw      = reqBody.value("pw", "");
    std::string name    = reqBody.value("name", "");
    std::string phone   = reqBody.value("phone", "");
    std::string address = reqBody.value("address", "");
    std::string role    = reqBody.value("role", "CUSTOMER");

    int newUserId = getInstance().queryInsertUser(loginId, pw, role, name, phone, address);
    if (newUserId != -1) {
        res["status"] = Status::SUCCESS;
        res["user_id"] = newUserId;
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "회원가입 DB 처리 실패";
    }
    return res;
}

nlohmann::json CommonDB::login(const nlohmann::json& reqBody) {
    nlohmann::json res;
    std::string loginId = reqBody.value("id", "");
    std::string pw      = reqBody.value("pw", "");
    std::string role    = reqBody.value("role", "CUSTOMER");
    
    // 클라이언트가 보낸 타입 백업 (1:고객, 2:사장, 3:라이더 등)
    int clientType = reqBody.value("client_type", 1); 

    int userId = getInstance().queryLogin(loginId, pw, role);
    if (userId != -1) {
        res["status"] = Status::SUCCESS;
        res["user_id"] = userId;
        res["client_type"] = clientType; // <--- 이 줄 추가! (CommonHandler가 필요로 함)
    } else {
        res["status"] = Status::UNAUTHORIZED;
        res["message"] = "아이디 또는 비밀번호가 올바르지 않습니다.";
    }
    return res;
}