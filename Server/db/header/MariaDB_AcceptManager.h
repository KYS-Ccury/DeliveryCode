#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <mariadb/mysql.h> // MariaDB C 커넥터 헤더

// 쿼리 결과를 담을 자료구조 (행별로 컬럼명:값을 std::map으로 저장)
using DBRow = std::map<std::string, std::string>;
using DBResult = std::vector<DBRow>;

class MariaDB_AcceptManager {
private:
    MYSQL* conn;          // MariaDB 연결 객체 포인터
    std::mutex db_mutex;  // 멀티스레드 환경에서 쿼리 충돌을 막기 위한 뮤텍스

    // 싱글톤 패턴을 위한 생성자/소멸자 은닉
    MariaDB_AcceptManager();
    ~MariaDB_AcceptManager();

    // 복사 생성자 및 대입 연산자 삭제 (싱글톤 보장)
    MariaDB_AcceptManager(const MariaDB_AcceptManager&) = delete;
    MariaDB_AcceptManager& operator=(const MariaDB_AcceptManager&) = delete;

public:
    // 전역에서 접근 가능한 유일한 인스턴스 반환
    static MariaDB_AcceptManager& getInstance();

    // --- [기본 DB 유틸리티 (접속 및 실행 전담)] ---
    bool connect(const std::string& host, const std::string& user, 
                 const std::string& password, const std::string& dbname, int port = 3306);
    void disconnect();
    
    DBResult executeQuery(const std::string& query);
    bool executeUpdate(const std::string& query);
    uint64_t getLastInsertId();
    
    // SQL 인젝션 방어용 이스케이프 함수
    std::string escapeStr(const std::string& s); 
};