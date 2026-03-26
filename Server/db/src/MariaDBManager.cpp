#include "MariaDBManager.h"
#include <iostream>

MariaDBManager& MariaDBManager::getInstance() {
    static MariaDBManager instance;
    return instance;
}

MariaDBManager::MariaDBManager() : conn(nullptr) {
    conn = mysql_init(nullptr);
    if (!conn)
        std::cerr << "[DB] mysql_init 실패" << std::endl;
}

MariaDBManager::~MariaDBManager() {
    disconnect();
}

bool MariaDBManager::connect(const std::string& host, const std::string& user,
                              const std::string& password, const std::string& dbname,
                              int port) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return false;

    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
                            dbname.c_str(), port, nullptr, 0)) {
        std::cerr << "[DB] 연결 실패: " << mysql_error(conn) << std::endl;
        return false;
    }
    mysql_set_character_set(conn, "utf8mb4");
    return true;
}

void MariaDBManager::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (conn) { mysql_close(conn); conn = nullptr; }
}

bool MariaDBManager::executeUpdate(const std::string& query) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return false;
    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "[DB] executeUpdate 실패: " << mysql_error(conn)
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
        std::cerr << "[DB] executeQuery 실패: " << mysql_error(conn)
                  << "\nQuery: " << query << std::endl;
        return result;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return result;

    int          num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields     = mysql_fetch_fields(res);
    MYSQL_ROW    row;
    while ((row = mysql_fetch_row(res))) {
        DBRow dbRow;
        for (int i = 0; i < num_fields; ++i)
            dbRow[fields[i].name] = row[i] ? row[i] : "";
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

// ── 트랜잭션 헬퍼 ──────────────────────────────────────────────
// recursive_mutex 덕분에 work() 안에서 executeQuery/executeUpdate
// 를 자유롭게 호출해도 데드락이 발생하지 않음
bool MariaDBManager::executeTransaction(const std::function<bool()>& work) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex);
    if (!conn) return false;

    if (mysql_query(conn, "START TRANSACTION") != 0) {
        std::cerr << "[DB] START TRANSACTION 실패: " << mysql_error(conn) << std::endl;
        return false;
    }
    try {
        if (work()) {
            mysql_query(conn, "COMMIT");
            return true;
        } else {
            mysql_query(conn, "ROLLBACK");
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB] 트랜잭션 예외: " << e.what() << std::endl;
        mysql_query(conn, "ROLLBACK");
        return false;
    }
}
