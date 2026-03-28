// OwnerDB_Store.cpp
#include "OwnerDB.h"
#include "Protocol.h"

using json = nlohmann::json;

// 1. 메뉴 목록 조회
json OwnerDB::getMenuList(int ownerId) {
    auto& db = MariaDBManager::getInstance();
    json res; res["menus"] = json::array();

    std::string query = 
        "SELECT m.menu_id, mc.category_name, m.menu_name, m.price, m.description, m.image_url "
        "FROM menus m "
        "JOIN menu_categories mc ON m.menu_category_id = mc.menu_category_id "
        "JOIN restaurants r ON mc.restaurant_id = r.restaurant_id "
        "WHERE r.owner_id = " + std::to_string(ownerId);

    auto rows = db.executeQuery(query);
    for (const auto& row : rows) {
        json item;
        item["menu_id"] = std::stoi(row.at("menu_id"));
        item["category"] = row.at("category_name");
        item["name"] = row.at("menu_name");
        item["price"] = std::stoi(row.at("price"));
        item["desc"] = row.at("description");
        item["image_url"] = row.at("image_url");
        res["menus"].push_back(item);
    }
    res["status"] = Status::SUCCESS;
    return res;
}

// 2. 메뉴 등록 (카테고리 자동 생성 포함)
json OwnerDB::addMenu(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    
    int ownerId = req.value("owner_id", 0);
    std::string category = req.value("category", "");
    std::string name = req.value("name", "");
    int price = req.value("price", 0);
    std::string desc = req.value("desc", "");
    std::string imgUrl = req.value("image_url", "");

    // 1) restaurant_id 찾기 (유령 계정 방어)
    auto rRows = db.executeQuery("SELECT restaurant_id FROM restaurants WHERE owner_id=" + std::to_string(ownerId));
    if (rRows.empty()) { 
        res["status"] = Status::NOT_FOUND; 
        res["message"] = "매장 정보가 DB에 없습니다. (과거 에러로 생성된 유령 계정일 수 있습니다. 새로 회원가입 해주세요!)";
        return res; 
    }
    std::string restId = rRows[0].at("restaurant_id");

    // 2) 카테고리가 없으면 만들고 id 가져오기
    std::string catQuery = "SELECT menu_category_id FROM menu_categories WHERE restaurant_id=" + restId + " AND category_name='" + MariaDBManager::escape(category) + "'";
    auto cRows = db.executeQuery(catQuery);
    std::string catId;
    
    if (cRows.empty()) {
        db.executeUpdate("INSERT INTO menu_categories (restaurant_id, category_name) VALUES (" + restId + ", '" + MariaDBManager::escape(category) + "')");
        // 🚨 방어 코드: getLastInsertId() 대신 직접 SELECT해서 가져오기
        auto newCatRows = db.executeQuery("SELECT menu_category_id FROM menu_categories WHERE restaurant_id=" + restId + " AND category_name='" + MariaDBManager::escape(category) + "'");
        if (!newCatRows.empty()) catId = newCatRows[0].at("menu_category_id");
        else { res["status"] = Status::SERVER_ERROR; res["message"] = "카테고리 생성 실패"; return res; }
    } else {
        catId = cRows[0].at("menu_category_id");
    }

    // 3) 메뉴 등록
    std::string insertQuery = "INSERT INTO menus (menu_category_id, menu_name, price, description, image_url) VALUES (" 
        + catId + ", '" + MariaDBManager::escape(name) + "', " + std::to_string(price) + ", '" 
        + MariaDBManager::escape(desc) + "', '" + MariaDBManager::escape(imgUrl) + "')";
        
    if (db.executeUpdate(insertQuery)) {
        res["status"] = Status::SUCCESS;
        // 🚨 방어 코드: 방금 넣은 메뉴 ID 직접 조회
        auto newMenuRows = db.executeQuery("SELECT MAX(menu_id) as new_id FROM menus WHERE menu_category_id=" + catId + " AND menu_name='" + MariaDBManager::escape(name) + "'");
        if (!newMenuRows.empty() && newMenuRows[0].count("new_id") && !newMenuRows[0].at("new_id").empty()) {
            res["menu_id"] = std::stoi(newMenuRows[0].at("new_id"));
        } else {
            res["menu_id"] = 0;
        }
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "메뉴 INSERT 쿼리 오류";
    }
    return res;
}

// 3. 메뉴 수정
json OwnerDB::updateMenu(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    int menuId = req.value("menu_id", 0);
    int ownerId = req.value("owner_id", 0); 
    std::string category = req.value("category", "");
    std::string name = req.value("name", "");
    int price = req.value("price", 0);
    std::string desc = req.value("desc", "");
    std::string imgUrl = req.value("image_url", "");

    auto rRows = db.executeQuery("SELECT restaurant_id FROM restaurants WHERE owner_id=" + std::to_string(ownerId));
    if (rRows.empty()) { res["status"] = Status::NOT_FOUND; res["message"] = "매장 정보가 없습니다."; return res; }
    std::string restId = rRows[0].at("restaurant_id");

    std::string catQuery = "SELECT menu_category_id FROM menu_categories WHERE restaurant_id=" + restId + " AND category_name='" + MariaDBManager::escape(category) + "'";
    auto cRows = db.executeQuery(catQuery);
    std::string catId;
    
    if (cRows.empty()) {
        db.executeUpdate("INSERT INTO menu_categories (restaurant_id, category_name) VALUES (" + restId + ", '" + MariaDBManager::escape(category) + "')");
        auto newCatRows = db.executeQuery("SELECT menu_category_id FROM menu_categories WHERE restaurant_id=" + restId + " AND category_name='" + MariaDBManager::escape(category) + "'");
        if (!newCatRows.empty()) catId = newCatRows[0].at("menu_category_id");
        else { res["status"] = Status::SERVER_ERROR; res["message"] = "카테고리 갱신 실패"; return res; }
    } else {
        catId = cRows[0].at("menu_category_id");
    }

    std::string query = "UPDATE menus SET menu_category_id=" + catId + 
                        ", menu_name='" + MariaDBManager::escape(name) + 
                        "', price=" + std::to_string(price) + 
                        ", description='" + MariaDBManager::escape(desc) + 
                        "', image_url='" + MariaDBManager::escape(imgUrl) + 
                        "' WHERE menu_id=" + std::to_string(menuId);
                        
    if (db.executeUpdate(query)) {
        res["status"] = Status::SUCCESS;
    } else {
        res["status"] = Status::SERVER_ERROR;
        res["message"] = "메뉴 UPDATE 쿼리 오류";
    }
    return res;
}

// 4. 메뉴 삭제
json OwnerDB::deleteMenu(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    int menuId = req.value("menu_id", 0);

    std::string query = "DELETE FROM menus WHERE menu_id=" + std::to_string(menuId);
    res["status"] = db.executeUpdate(query) ? Status::SUCCESS : Status::SERVER_ERROR;
    return res;
}

json OwnerDB::updateStoreStatus(const json& req) {
    auto& db = MariaDBManager::getInstance();
    json res;
    
    int ownerId = req.value("owner_id", -1);
    bool isOpen = req.value("is_open", false);

    // restaurants 테이블의 is_open 컬럼을 업데이트 (사장님 ID 기준)
    std::string query = "UPDATE restaurants SET is_open=" + std::to_string(isOpen ? 1 : 0) + 
                        " WHERE owner_id=" + std::to_string(ownerId);

    if (db.executeUpdate(query)) {
        res["status"] = Status::SUCCESS;
        res["is_open"] = isOpen; // 변경된 상태 반환
    } else {
        res["status"] = Status::SERVER_ERROR;
    }
    
    return res;
}