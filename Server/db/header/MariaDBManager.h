#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <mariadb/mysql.h>

using DBRow    = std::map<std::string, std::string>;
using DBResult = std::vector<DBRow>;

class MariaDBManager {
private:
    MYSQL* conn;
    // recursive_mutex: 같은 스레드에서 중복 잠금 허용 (트랜잭션 내 쿼리 호출 지원)
    std::recursive_mutex db_mutex;

    MariaDBManager();
    ~MariaDBManager();
    MariaDBManager(const MariaDBManager&) = delete;
    MariaDBManager& operator=(const MariaDBManager&) = delete;

public:
    static MariaDBManager& getInstance();

    bool     connect(const std::string& host, const std::string& user,
                     const std::string& password, const std::string& dbname,
                     int port = 3306);
    void     disconnect();

    DBResult executeQuery(const std::string& query);
    bool     executeUpdate(const std::string& query);
    uint64_t getLastInsertId();

    // ── 트랜잭션 헬퍼 ─────────────────────────────────────
    // 람다 안에서 executeQuery/executeUpdate 를 자유롭게 호출 가능
    // 예외 발생 시 자동 ROLLBACK
    bool executeTransaction(const std::function<bool()>& work);

    // ── 공통 SQL 이스케이프 ───────────────────────────────
    // CommonDB / RiderDB / BaseHandler 모두 이 함수 하나만 사용
    static std::string escape(const std::string& s);
};