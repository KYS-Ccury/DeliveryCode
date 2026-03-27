#include "MariaDBManager.h"
#include <iostream>

MariaDBManager& MariaDBManager::getInstance() {
    static MariaDBManager instance;
    return instance;
}

MariaDBManager::MariaDBManager() : conn(nullptr) {
    conn = mysql_init(nullptr);
    if (conn == nullptr) {
        std::cerr << "[MariaDBManager] mysql_init 실패" << std::endl;
    }
}

MariaDBManager::~MariaDBManager() {
    disconnect();
}

bool MariaDBManager::connect(const std::string& host, const std::string& user,
                              const std::string& password, const std::string& dbname,
                              int port) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);

    if (conn == nullptr) return false;

    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
                           dbname.c_str(), port, nullptr, 0) == nullptr) {
        std::cerr << "[MariaDBManager] 연결 실패: " << mysql_error(conn) << std::endl;
        return false;
    }

    mysql_set_character_set(conn, "utf8mb4");
    std::cout << "[MariaDBManager] DB 연결 성공" << std::endl;
    return true;
}

void MariaDBManager::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (conn) {mysql_close(conn); conn = nullptr;
    }
}

bool MariaDBManager::executeUpdate(const std::string& query) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return false;
    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "[MariaDBManager] UPDATE 실패: " << mysql_error(conn)
                  << "\nQuery: " << query << std::endl;
        return false;
    }
    return true;
}

DBResult MariaDBManager::executeQuery(const std::string& query) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    DBResult result;
    if (!conn) return result;

    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "[MariaDBManager] SELECT 실패: " << mysql_error(conn)
                  << "\nQuery: " << query << std::endl;
        return result;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) return result;

    int         num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields    = mysql_fetch_fields(res);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        DBRow dbRow;
        for (int i = 0; i < num_fields; i++) {
            dbRow[fields[i].name] = row[i] ? row[i] : "";        }
        result.push_back(dbRow);
    }
    mysql_free_result(res);
    return result;
}

uint64_t MariaDBManager::getLastInsertId() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return 0;
    return mysql_insert_id(conn);
}

bool MariaDBManager::executeTransaction(const std::function<bool()>& work) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return false;

    if (mysql_query(conn, "START TRANSACTION") != 0) {
        std::cerr << "[MariaDBManager] START TRANSACTION 실패: " << mysql_error(conn) << std::endl;
        return false;
    }
    try {
        if (!work()) {
            mysql_query(conn, "ROLLBACK");
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[MariaDBManager] 트랜잭션 예외: " << e.what() << std::endl;
        mysql_query(conn, "ROLLBACK");
        return false;
    }
    if (mysql_query(conn, "COMMIT") != 0) {
        std::cerr << "[MariaDBManager] COMMIT 실패: " << mysql_error(conn) << std::endl;
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    return true;
}


std::string MariaDBManager::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (c == '\'' || c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

// 안전한 SQL 문자열 이스케이프 처리 (MariaDB 내장 기능 사용)
std::string MariaDBManager::escapeStr(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return s; 

    // 최악의 경우 길이가 2배 + 1바이트(널 문자) 길어질 수 있음
    char* buffer = new char[s.length() * 2 + 1];
    mysql_real_escape_string(conn, buffer, s.c_str(), s.length());
    std::string result(buffer);
    delete[] buffer;
    return result;
}