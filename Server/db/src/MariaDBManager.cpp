#include "MariaDBManager.h"
#include <iostream>

// 싱글톤 인스턴스 생성 및 반환
MariaDBManager& MariaDBManager::getInstance() {
    static MariaDBManager instance;
    return instance;
}

// 생성자: 연결 객체 초기화
MariaDBManager::MariaDBManager() : conn(nullptr) {
    conn = mysql_init(nullptr);
    if (conn == nullptr) {
        std::cerr << "[DB Error] mysql_init 실패" << std::endl;
    }
}

// 소멸자: 프로그램 종료 시 자동 연결 해제
MariaDBManager::~MariaDBManager() {
    disconnect();
}

// DB 연결
bool MariaDBManager::connect(const std::string& host, const std::string& user, 
                             const std::string& password, const std::string& dbname, int port) {
    std::lock_guard<std::mutex> lock(db_mutex);

    if (conn == nullptr) return false;

    // mysql_real_connect 호출
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), 
                           dbname.c_str(), port, nullptr, 0) == nullptr) {
        std::cerr << "[DB Error] 연결 실패: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 한글 처리를 위해 utf8mb4 셋팅 (스키마 요구사항 반영)
    mysql_set_character_set(conn, "utf8mb4");
    return true;
}

// DB 연결 해제
void MariaDBManager::disconnect() {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

// INSERT / UPDATE / DELETE 실행 (결과셋이 없는 쿼리)
bool MariaDBManager::executeUpdate(const std::string& query) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if (!conn) return false;

    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "[DB Error] 쿼리 실행 실패: " << mysql_error(conn) << "\nQuery: " << query << std::endl;
        return false;
    }
    return true;
}

// SELECT 실행 (결과셋이 있는 쿼리)
DBResult MariaDBManager::executeQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(db_mutex);
    DBResult result;

    if (!conn) return result;

    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "[DB Error] SELECT 쿼리 실패: " << mysql_error(conn) << "\nQuery: " << query << std::endl;
        return result;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        // 결과셋이 없는 경우(에러거나 0건)
        return result;
    }

    // 컬럼(필드) 이름들 가져오기
    int num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    // 행(Row) 데이터를 돌면서 map 구조로 저장
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        DBRow dbRow;
        for (int i = 0; i < num_fields; i++) {
            std::string columnName = fields[i].name;
            // null 값인 경우 빈 문자열("")로 처리
            std::string columnValue = row[i] ? row[i] : ""; 
            dbRow[columnName] = columnValue;
        }
        result.push_back(dbRow);
    }

    // 메모리 해제
    mysql_free_result(res);
    return result;
}

// 마지막에 삽입된 행의 PK(AUTO_INCREMENT) 값 반환
uint64_t MariaDBManager::getLastInsertId() {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!conn) return 0;
    return mysql_insert_id(conn);
}