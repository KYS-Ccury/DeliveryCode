#include "CommonDB.h"
#include <iostream>

// ── 내부 유틸 ──────────────────────────────────────────────
std::string CommonDB::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// ── queryLogin ─────────────────────────────────────────────
int CommonDB::queryLogin(const std::string& loginId,
                         const std::string& password,
                         const std::string& role) {
    auto& db = MariaDBManager::getInstance();
    std::string q =
        "SELECT user_id FROM users "
        "WHERE login_id='"  + escape(loginId)  + "' "
        "  AND password='"  + escape(password) + "' "
        "  AND role='"      + escape(role)      + "' "
        "  AND status='ACTIVE' LIMIT 1";

    auto rows = db.executeQuery(q);
    if (rows.empty()) return -1;

    try { return std::stoi(rows[0].at("user_id")); }
    catch (...) { return -1; }
}

// ── querySignup ────────────────────────────────────────────
int CommonDB::querySignup(const std::string& loginId,
                          const std::string& password,
                          const std::string& role,
                          const std::string& name,
                          const std::string& phone,
                          const std::string& address) {
    auto& db = MariaDBManager::getInstance();

    // 중복 ID 체크
    auto dup = db.executeQuery(
        "SELECT user_id FROM users WHERE login_id='" + escape(loginId) + "' LIMIT 1");
    if (!dup.empty()) return -1;   // 이미 존재

    bool ok = db.executeUpdate(
        "INSERT INTO users (login_id, password, role, name, phone, address, status) "
        "VALUES ('"
        + escape(loginId)  + "','"
        + escape(password) + "','"
        + escape(role)     + "','"
        + escape(name)     + "','"
        + escape(phone)    + "','"
        + escape(address)  + "',"
        "'ACTIVE')");

    if (!ok) return -1;

    try { return static_cast<int>(db.getLastInsertId()); }
    catch (...) { return -1; }
}

// ── queryUserBasic ─────────────────────────────────────────
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

// ── queryCheckPassword ─────────────────────────────────────
bool CommonDB::queryCheckPassword(int userId, const std::string& password) {
    auto& db = MariaDBManager::getInstance();
    auto rows = db.executeQuery(
        "SELECT user_id FROM users "
        "WHERE user_id=" + std::to_string(userId) +
        "  AND password='" + escape(password) + "' LIMIT 1");
    return !rows.empty();
}

// ── queryChangePassword ────────────────────────────────────
bool CommonDB::queryChangePassword(int userId, const std::string& newPassword) {
    auto& db = MariaDBManager::getInstance();
    return db.executeUpdate(
        "UPDATE users SET password='" + escape(newPassword) +
        "' WHERE user_id=" + std::to_string(userId));
}

// ── querySetActive ─────────────────────────────────────────
bool CommonDB::querySetActive(int userId, bool active) {
    auto& db = MariaDBManager::getInstance();
    std::string val = active ? "'ACTIVE'" : "'SLEEP'";
    return db.executeUpdate(
        "UPDATE users SET status=" + val +
        " WHERE user_id=" + std::to_string(userId));
}
