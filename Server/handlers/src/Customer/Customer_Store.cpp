#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <cmath>

#include "CommonDB.h"

#include "CommonDB.h"

using json = nlohmann::json;

static double calcDistanceKm(double lat1, double lng1, double lat2, double lng2) {
    const double R = 6371.0, PI = 3.14159265358979;
    double dLat = (lat2-lat1)*PI/180.0, dLng = (lng2-lng1)*PI/180.0;
    double a = std::sin(dLat/2)*std::sin(dLat/2)
             + std::cos(lat1*PI/180.0)*std::cos(lat2*PI/180.0)
             * std::sin(dLng/2)*std::sin(dLng/2);
    return R * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0-a));
}
static std::string estimateDeliveryTime(double km) {
    int lo = ((15 + (int)(km*4)) / 5) * 5;
    return std::to_string(lo) + "~" + std::to_string(lo+10) + "분";
}
static std::string formatFee(int fee) {
    if (fee <= 0) return "무료";
    std::string s = std::to_string(fee);
    if (s.size() > 3) s.insert(s.size()-3, ",");
    return s + "원";
}

void CustomerHandler::handleStoreList(Session* session, const std::string& body) {
    try {
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) { sendError(session, CmdCustomer::REQ_STORE_LIST, Status::UNAUTHORIZED, "로그인 필요"); return; }

        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body.empty() ? "{}" : body);
        std::string category = req.value("category", "");
        double userLat = req.value("user_lat", 35.1468);
        double userLng = req.value("user_lng", 126.9227);

        // std::string q =
        //     "SELECT r.restaurant_id AS id, r.restaurant_name AS name, "
        //     "fc.category_name AS category, r.base_delivery_fee AS delivery_fee, "
        //     "r.min_order_amt, r.rating_avg AS rating, r.address, r.phone, "
        //     "r.notice AS description, r.latitude, r.longitude, r.logo_url, "
        //     "r.business_hours AS open_time, r.holiday "
        //     "FROM restaurants r "
        //     "JOIN food_categories fc ON fc.category_id = r.category_id "
        //     "WHERE r.is_open = TRUE ";
        // if (!category.empty() && category != "전체")
        // q += "ORDER BY r.rating_avg DESC";

        std::string q =
            "SELECT r.restaurant_id AS id, r.restaurant_name AS name, "
            "fc.category_name AS category, r.base_delivery_fee AS delivery_fee, "
            "r.min_order_amt, r.rating_avg AS rating, r.address, r.phone, "
            "r.notice AS description, r.latitude, r.longitude, r.logo_url, "
            "r.business_hours AS open_time, r.holiday "
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.is_open = TRUE ";

        // ✅ 수정: 카테고리 필터 조건 추가
        if (!category.empty() && category != "전체")
            q += "AND fc.category_name = '" + category + "' ";

        q += "ORDER BY r.rating_avg DESC";

        auto rows = db.executeQuery(q);
        json stores = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]       = std::stoi(r.at("id"));
            s["name"]     = r.at("name");
            s["category"] = r.count("category") ? r.at("category") : "";
            s["address"]  = r.count("address")  ? r.at("address")  : "";
            s["phone"]    = r.count("phone")     ? r.at("phone")    : "";
            s["description"] = r.count("description") ? r.at("description") : "";
            s["open_time"]   = r.count("open_time") && !r.at("open_time").empty() ? r.at("open_time") : "";
            s["holiday"]     = r.count("holiday")   && !r.at("holiday").empty()   ? r.at("holiday")   : "";

            int fee = r.count("delivery_fee") && !r.at("delivery_fee").empty() ? std::stoi(r.at("delivery_fee")) : 0;
            s["delivery_fee"]     = fee;
            s["delivery_fee_str"] = formatFee(fee);

            s["min_order"] = r.count("min_order_amt") && !r.at("min_order_amt").empty() ? std::stoi(r.at("min_order_amt")) : 0;
            s["rating"]    = r.count("rating") && !r.at("rating").empty() ? std::stod(r.at("rating")) : 0.0;

            double sLat = r.count("latitude")  && !r.at("latitude").empty()  ? std::stod(r.at("latitude"))  : 0.0;
            double sLng = r.count("longitude") && !r.at("longitude").empty() ? std::stod(r.at("longitude")) : 0.0;
            double km = (sLat!=0.0 && sLng!=0.0) ? std::round(calcDistanceKm(userLat,userLng,sLat,sLng)*10)/10.0 : 0.0;
            s["distance"]      = km;
            s["delivery_time"] = (km > 0.0) ? estimateDeliveryTime(km) : "20~30분";

            // 가게 대표 이미지: 해당 가게의 첫 번째 메뉴 이미지 사용
            int rid = s["id"].get<int>();
            auto imgRows = db.executeQuery(
                "SELECT m.image_url FROM menus m "
                "JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
                "WHERE mc.restaurant_id=" + std::to_string(rid) +
                "  AND m.image_url IS NOT NULL AND m.image_url != '' "
                "ORDER BY m.menu_id LIMIT 1");
            // ★ image_url이 없으면 placeholder_<restaurant_id> 사용
            //   서버가 REQ_GET_IMAGE(217)로 요청받으면 컬러 PNG를 동적 생성
            if (!imgRows.empty() && imgRows[0].count("image_url") &&
                !imgRows[0].at("image_url").empty()) {
                s["image_url"] = imgRows[0].at("image_url");
            } else {
                s["image_url"] = "placeholder_" + std::to_string(rid);
            }
            // ★ 로고 URL (restaurants.logo_url)
            s["logo_url"] = (r.count("logo_url") && !r.at("logo_url").empty())
                            ? r.at("logo_url") : "";

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
        if (uid <= 0) { sendError(session, CmdCustomer::REQ_MENU_LIST, Status::UNAUTHORIZED, "로그인 필요"); return; }

        auto& db = MariaDBManager::getInstance();
        json req = json::parse(body.empty() ? "{}" : body);
        int storeID = req.value("store_id", 0);
        std::string subCat = req.value("sub_category", "");
        if (!storeID) { sendError(session, CmdCustomer::REQ_MENU_LIST, Status::BAD_REQUEST, "store_id 필요"); return; }

        auto catRows = db.executeQuery(
            "SELECT mc.menu_category_id, mc.category_name FROM menu_categories mc "
            "WHERE mc.restaurant_id=" + std::to_string(storeID) +
            " ORDER BY mc.sort_order, mc.menu_category_id");

        std::string menuQ =
            "SELECT m.menu_id, m.menu_name, m.description, m.price, "
            "m.image_url, m.is_sold_out, mc.category_name AS sub_category "
            "FROM menus m JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
            "WHERE mc.restaurant_id=" + std::to_string(storeID);
        if (!subCat.empty() && subCat != "전체")
            menuQ += " AND mc.category_name='" + CommonDB::getInstance().escape(subCat) + "'";
        menuQ += " ORDER BY mc.sort_order, mc.menu_category_id, m.menu_id";

        auto menuRows = db.executeQuery(menuQ);
        json menus = json::array();
        for (auto& mr : menuRows) {
            int menuID = std::stoi(mr.at("menu_id"));
            json m;
            m["id"]          = menuID;
            m["name"]        = mr.at("menu_name");
            m["desc"]        = mr.count("description") ? mr.at("description") : "";
            m["price"]       = std::stoi(mr.at("price"));
            m["image_url"]   = mr.count("image_url")   ? mr.at("image_url")   : "";
            m["is_sold_out"] = (mr.count("is_sold_out") && mr.at("is_sold_out") == "1");
            m["sub_category"]= mr.at("sub_category");

            auto ogRows = db.executeQuery(
                "SELECT og.option_group_id, og.group_name, og.is_essential, og.max_select "
                "FROM option_groups og WHERE og.menu_id=" + std::to_string(menuID) +
                " ORDER BY og.option_group_id");

            json optGroups = json::array();
            for (auto& og : ogRows) {
                int ogID = std::stoi(og.at("option_group_id"));
                json grp;
                grp["group_id"]   = ogID;
                grp["group_name"] = og.at("group_name");
                grp["is_required"]= (og.count("is_essential") && og.at("is_essential") == "1");
                grp["max_select"] = std::stoi(og.at("max_select"));
                auto oiRows = db.executeQuery(
                    "SELECT option_item_id, option_name, extra_price FROM option_items "
                    "WHERE option_group_id=" + std::to_string(ogID) + " ORDER BY option_item_id");
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

        json res;
        res["status"] = Status::SUCCESS; res["menus"] = menus; res["sub_categories"] = subCats;
        session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::REQ_MENU_LIST, res.dump());
    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_MENU_LIST, Status::SERVER_ERROR, e.what());
    }
}
