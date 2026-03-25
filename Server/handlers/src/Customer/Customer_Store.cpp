#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"

using json = nlohmann::json;

void CustomerHandler::handleStoreList(Session* session, const std::string& body) {
    try {
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) {
            sendError(session, CmdCustomer::REQ_STORE_LIST, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);
        std::string category = req.value("category", "전체");

        std::string q =
            "SELECT r.restaurant_id AS id, r.restaurant_name AS name, "
            "       fc.category_name AS category, r.base_delivery_fee AS delivery_fee, "
            "       r.min_order_amt, r.rating_avg AS rating, r.address, r.phone, "
            "       r.notice AS description "
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.is_open = TRUE ";
        if (category != "전체")
            q += "AND fc.category_name='" + escapeStr(category) + "' ";
        q += "ORDER BY r.rating_avg DESC";

        auto rows = db.executeQuery(q);
        json stores = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]           = std::stoi(r.at("id"));
            s["name"]         = r.at("name");
            s["category"]     = r.at("category");
            s["delivery_time"]= "20~40분";
            s["min_order"]    = std::stoi(r.count("min_order_amt") ? r.at("min_order_amt") : "0");
            s["delivery_fee"] = std::stoi(r.count("delivery_fee")  ? r.at("delivery_fee")  : "0");
            s["rating"]       = r.count("rating") && !r.at("rating").empty() ? std::stod(r.at("rating")) : 0.0;
            s["address"]      = r.count("address") ? r.at("address") : "";
            s["phone"]        = r.count("phone")   ? r.at("phone")   : "";
            s["description"]  = r.count("description") ? r.at("description") : "";
            stores.push_back(s);
        }

        json res; res["status"] = Status::SUCCESS; res["stores"] = stores;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_STORE_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_STORE_LIST, Status::SERVER_ERROR, e.what());
    }
}

void CustomerHandler::handleMenuList(Session* session, const std::string& body) {
    try {

        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) {
            sendError(session, CmdCustomer::REQ_MENU_LIST, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }
        
        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body);

        int         storeID = req.value("store_id",     0);
        std::string subCat  = req.value("sub_category", "전체");

        if (!storeID) { sendError(session, CmdCustomer::REQ_MENU_LIST, Status::BAD_REQUEST, "store_id 필요"); return; }

        std::string catQ = "SELECT mc.menu_category_id, mc.category_name FROM menu_categories mc WHERE mc.restaurant_id=" + std::to_string(storeID) + " ORDER BY mc.menu_category_id";
        auto catRows = db.executeQuery(catQ);

        std::string menuQ = "SELECT m.menu_id, m.menu_name, m.description, m.price, m.is_sold_out, mc.category_name AS sub_category "
                            "FROM menus m JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
                            "WHERE mc.restaurant_id=" + std::to_string(storeID);
        if (subCat != "전체") menuQ += " AND mc.category_name='" + escapeStr(subCat) + "'";
        menuQ += " ORDER BY mc.menu_category_id, m.menu_id";

        auto menuRows = db.executeQuery(menuQ);
        json menus = json::array();

        for (auto& mr : menuRows) {
            int menuID = std::stoi(mr.at("menu_id"));
            json m;
            m["id"]           = menuID;
            m["name"]         = mr.at("menu_name");
            m["desc"]         = mr.count("description") ? mr.at("description") : "";
            m["price"]        = std::stoi(mr.at("price"));
            m["is_sold_out"]  = (mr.at("is_sold_out") == "1");
            m["sub_category"] = mr.at("sub_category");

            auto ogRows = db.executeQuery(
                "SELECT og.option_group_id, og.group_name, og.is_essential, og.max_select "
                "FROM option_groups og WHERE og.menu_id=" + std::to_string(menuID));

            json optGroups = json::array();
            for (auto& og : ogRows) {
                int ogID = std::stoi(og.at("option_group_id"));
                json grp;
                grp["group_id"]   = ogID;
                grp["group_name"] = og.at("group_name");
                grp["is_required"]= (og.at("is_essential") == "1");
                grp["max_select"] = std::stoi(og.at("max_select"));

            auto oiRows = db.executeQuery(
                "SELECT option_item_id, option_name, extra_price "
                "FROM option_items WHERE option_group_id=" + std::to_string(ogID));
            json items = json::array();
            for (auto& oi : oiRows) {
                json opt;
                opt["option_id"] = std::stoi(oi.at("option_item_id"));
                opt["name"]      = oi.at("option_name");
                opt["price"]     = std::stoi(oi.at("extra_price"));
                items.push_back(opt);
            }
                grp["options"] = items;
                optGroups.push_back(grp);
            }
            m["option_groups"] = optGroups;
            menus.push_back(m);
        }

        json subCats = json::array();
        subCats.push_back("전체");
        for (auto& cr : catRows) subCats.push_back(cr.at("category_name"));

        json res; res["status"] = Status::SUCCESS; res["menus"] = menus; res["sub_categories"] = subCats;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_MENU_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_MENU_LIST, Status::SERVER_ERROR, e.what());
    }
}