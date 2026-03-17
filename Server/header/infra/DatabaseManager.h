//  데이터베이스 접근을 담당하는 매니저이다.
//  지금은 뼈대만 두고 나중에 MariaDB 또는 SQLite 연결을 붙일 수 있게 설계한다.

#pragma once

#include <string>
#include <mutex>

class DatabaseManager {
public:
    //  DB 연결을 시도한다.
    bool connect(const std::string& connStr);

    //  로그성 저장 예시 함수이다.
    void saveOrderLog(int orderId, const std::string& message);

private:
    //  DB 보호용 뮤텍스이다.
    std::mutex m_mtx;

    //  연결 여부이다.
    bool m_connected = false;
};