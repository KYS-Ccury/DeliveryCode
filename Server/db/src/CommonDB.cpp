#include "CommonDB.h"
#include <iostream>

// ================================================================
//  escape  (공통 SQL 이스케이프 유틸)
//  CommonDB::escape(str) 으로 어디서든 호출
// ================================================================
std::string CommonDB::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// ── 내부 유틸 ──────────────────────────────────────────────
// ============================================================
//  [로그인]
// ============================================================
int CommonDB::queryLogin(const std::string& loginId,
                         const std::string& password,
                         const std::string& role) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE login_id='"  + CommonDB::escape(loginId)  + "' "
        "  AND password='"  + CommonDB::escape(password) + "' "
        "  AND role='"      + CommonDB::escape(role)      + "' "
        "  AND status='ACTIVE' LIMIT 1");

    if (rows.empty()) return -1;
    try { return std::stoi(rows[0].at("user_id")); }
    catch (...) { return -1; }
}

// ============================================================
//  [회원가입]
// ============================================================
bool CommonDB::queryCheckDuplicateId(const std::string& loginId) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE login_id='" + CommonDB::escape(loginId) + "' LIMIT 1");
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
        + CommonDB::escape(loginId)  + "','"
        + CommonDB::escape(password) + "','"
        + CommonDB::escape(role)     + "','"
        + CommonDB::escape(name)     + "','"
        + CommonDB::escape(phone)    + "','"
        + CommonDB::escape(address)  + "',"
        "'ACTIVE')");

    if (!ok) return -1;
    try { return static_cast<int>(db.getLastInsertId()); }
    catch (...) { return -1; }
}

// ============================================================
//  [기본 정보 조회]
// ============================================================
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

// ============================================================
//  [비밀번호]
// ============================================================
bool CommonDB::queryCheckPassword(int userId, const std::string& password) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE user_id=" + std::to_string(userId) +
        "  AND password='" + CommonDB::escape(password) + "' LIMIT 1");
    return !rows.empty();
}

bool CommonDB::queryChangePassword(int userId, const std::string& newPassword) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE users SET password='" + CommonDB::escape(newPassword) +
        "' WHERE user_id=" + std::to_string(userId));
}

// ============================================================
//  [결제수단]
// ============================================================
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
        + CommonDB::escape(methodType) + "','"
        + CommonDB::escape(alias)      + "','"
        + CommonDB::escape(maskedNum)  + "',0)");
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

// ============================================================
//  [계정 상태]
// ============================================================
bool CommonDB::querySetUserStatus(int userId, const std::string& status) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE users SET status='" + CommonDB::escape(status) +
        "' WHERE user_id=" + std::to_string(userId));
}
