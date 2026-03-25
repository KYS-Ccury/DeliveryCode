#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <mariadb/mysql.h> // MariaDB C 커넥터 헤더

// 쿼리 결과를 담을 자료구조 (예: 행별로 컬럼명:값을 std::map으로 저장)
using DBRow = std::map<std::string, std::string>;
using DBResult = std::vector<DBRow>;

class MariaDBManager {
private:
    MYSQL* conn;          // MariaDB 연결 객체 포인터
    std::mutex db_mutex;  // 멀티스레드 환경에서 쿼리 충돌을 막기 위한 뮤텍스

    // 싱글톤 패턴을 위한 생성자/소멸자 은닉
    MariaDBManager();
    ~MariaDBManager();

    // 복사 생성자 및 대입 연산자 삭제 (싱글톤 보장)
    MariaDBManager(const MariaDBManager&) = delete;
    MariaDBManager& operator=(const MariaDBManager&) = delete;

public:
    // 전역에서 접근 가능한 유일한 인스턴스 반환
    static MariaDBManager& getInstance();

    // DB 연결 함수 (main.cpp에서 호출)
    bool connect(const std::string& host, const std::string& user, 
                 const std::string& password, const std::string& dbname, int port = 3306);
                 
    // DB 연결 해제
    void disconnect();

    // 1. SELECT 쿼리 실행 (결과셋 반환)
    // 예: "SELECT * FROM users WHERE role = 'RIDER'"
    DBResult executeQuery(const std::string& query);

    // 2. INSERT / UPDATE / DELETE 쿼리 실행 (성공 여부 반환)
    // 예: "UPDATE orders SET status = 'COOKING' WHERE order_id = 1"
    bool executeUpdate(const std::string& query);
    
    // (선택) 마지막으로 INSERT된 AUTO_INCREMENT ID 가져오기
    uint64_t getLastInsertId();
};