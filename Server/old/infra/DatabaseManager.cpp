//  데이터베이스 매니저 구현부이다.

#include "DatabaseManager.h"
#include <iostream>

//  DB 연결을 시도한다.
bool DatabaseManager::connect(const std::string& connStr) {
    //  실제 구현 대신 연결 성공 로그만 남긴다.
    std::lock_guard<std::mutex> lock(m_mtx);
    m_connected = true;
    std::cout << "[DB] connected: " << connStr << std::endl;
    return true;
}

//  주문 로그를 저장한다.
void DatabaseManager::saveOrderLog(int orderId, const std::string& message) {
    //  실제 DB INSERT 대신 콘솔에 출력한다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  연결되지 않았으면 저장하지 않는다.
    if (!m_connected) {
        std::cout << "[DB] not connected" << std::endl;
        return;
    }

    //  로그를 출력한다.
    std::cout << "[DB] orderId=" << orderId << " msg=" << message << std::endl;
}