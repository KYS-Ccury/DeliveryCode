//  ?곗씠?곕쿋?댁뒪 留ㅻ땲? 援ы쁽遺?대떎.

#include "DatabaseManager.h"
#include <iostream>

//  DB ?곌껐???쒕룄?쒕떎.
bool DatabaseManager::connect(const std::string& connStr) {
    //  ?ㅼ젣 援ы쁽 ????곌껐 ?깃났 濡쒓렇留??④릿??
    std::lock_guard<std::mutex> lock(m_mtx);
    m_connected = true;
    std::cout << "[DB] connected: " << connStr << std::endl;
    return true;
}

//  二쇰Ц 濡쒓렇瑜???ν븳??
void DatabaseManager::saveOrderLog(int orderId, const std::string& message) {
    //  ?ㅼ젣 DB INSERT ???肄섏넄??異쒕젰?쒕떎.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  ?곌껐?섏? ?딆븯?쇰㈃ ??ν븯吏 ?딅뒗??
    if (!m_connected) {
        std::cout << "[DB] not connected" << std::endl;
        return;
    }

    //  濡쒓렇瑜?異쒕젰?쒕떎.
    std::cout << "[DB] orderId=" << orderId << " msg=" << message << std::endl;
}