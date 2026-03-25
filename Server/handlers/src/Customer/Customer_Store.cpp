#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <cmath>   // sqrt, pow

using json = nlohmann::json;

// ── 위도/경도 → 거리(km) 계산 (Haversine 근사) ────────────────
static double calcDistanceKm(double lat1, double lng1,
                              double lat2, double lng2) {
    const double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLng = (lng2 - lng1) * M_PI / 180.0;
    double a = std::sin(dLat/2)*std::sin(dLat/2)
             + std::cos(lat1*M_PI/180.0)*std::cos(lat2*M_PI/180.0)
             * std::sin(dLng/2)*std::sin(dLng/2);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0-a));
    return R * c;
}

// ── 거리(km) → 예상 배달시간 문자열 ──────────────────────────
static std::string estimateDeliveryTime(double distKm) {
    // 기본 준비 시간 15분 + km당 약 4분 (오토바이 기준)
    int minutes = 15 + static_cast<int>(distKm * 4.0);
    int lo = (minutes / 5) * 5;        // 5분 단위 내림
    int hi = lo + 10;                  // 범위 상한
    return std::to_string(lo) + "~" + std::to_string(hi) + "분";
}

// ── 배달비 숫자 → "무료" or "X,XXX원" 문자열 ──────────────────
static std::string formatDeliveryFee(int fee) {
    if (fee <= 0) return "무료";
    // 천 단위 콤마 포맷
    std::string s = std::to_string(fee);
    if (s.size() > 3)
        s.insert(s.size() - 3, ",");
    return s + "원";
}

void CustomerHandler::handleStoreList(Session* session, const std::string& body) {
    try {
        int uid = getUserIdByFd(session->getFd());
        if (uid <= 0) {
            sendError(session, CmdCustomer::REQ_STORE_LIST, Status::UNAUTHORIZED, "로그인 필요");
            return;
        }

        auto& db = MariaDBManager::getInstance();
        json  req = json::parse(body.empty() ? "{}" : body);

        std::string category = req.value("category", "");

        // ★ 고객 위치: 클라이언트가 보내면 사용, 없으면 광주 중심(광주시청)
        double userLat = req.value("user_lat", 35.1595);
        double userLng = req.value("user_lng", 126.8526);

        std::string q =
            "SELECT r.restaurant_id AS id, r.restaurant_name AS name, "
            "       fc.category_name AS category, "
            "       r.base_delivery_fee AS delivery_fee, "
            "       r.min_order_amt, r.rating_avg AS rating, "
            "       r.address, r.phone, r.notice AS description, "
            "       r.latitude, r.longitude "             // ★ 추가
            "FROM restaurants r "
            "JOIN food_categories fc ON fc.category_id = r.category_id "
            "WHERE r.is_open = TRUE ";

        if (!category.empty() && category != "전체")
            q += "AND fc.category_name='" + escapeStr(category) + "' ";

        q += "ORDER BY r.rating_avg DESC";

        auto rows = db.executeQuery(q);
        json stores = json::array();
        for (auto& r : rows) {
            json s;
            s["id"]          = std::stoi(r.at("id"));
            s["name"]        = r.at("name");
            s["category"]    = r.count("category")     ? r.at("category")    : "";
            s["address"]     = r.count("address")      ? r.at("address")     : "";
            s["phone"]       = r.count("phone")        ? r.at("phone")       : "";
            s["description"] = r.count("description")  ? r.at("description") : "";

            // ★ 배달비: 숫자(int)와 문자열(string) 둘 다 전송
            int feeInt = (r.count("delivery_fee") && !r.at("delivery_fee").empty())
                         ? std::stoi(r.at("delivery_fee")) : 0;
            s["delivery_fee"]     = feeInt;
            s["delivery_fee_str"] = formatDeliveryFee(feeInt);   // "무료" or "2,000원"

            s["min_order"] = (r.count("min_order_amt") && !r.at("min_order_amt").empty())
                              ? std::stoi(r.at("min_order_amt")) : 0;

            s["rating"] = (r.count("rating") && !r.at("rating").empty())
                           ? std::stod(r.at("rating")) : 0.0;

            // ★ 거리 계산
            double storeLat = (r.count("latitude")  && !r.at("latitude").empty())
                               ? std::stod(r.at("latitude"))  : userLat;
            double storeLng = (r.count("longitude") && !r.at("longitude").empty())
                               ? std::stod(r.at("longitude")) : userLng;

            double distKm = calcDistanceKm(userLat, userLng, storeLat, storeLng);
            // 소수점 1자리로 반올림
            distKm = std::round(distKm * 10.0) / 10.0;

            s["distance"]      = distKm;                           // 숫자 (km)
            s["delivery_time"] = estimateDeliveryTime(distKm);     // ★ "25~35분" 등 동적 계산

            stores.push_back(s);
        }

        json res;
        res["status"] = Status::SUCCESS;
        res["stores"] = stores;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_STORE_LIST, res.dump());

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
        json  req = json::parse(body.empty() ? "{}" : body);

        int         storeID = req.value("store_id",     0);
        std::string subCat  = req.value("sub_category", "");

        if (!storeID) {
            sendError(session, CmdCustomer::REQ_MENU_LIST, Status::BAD_REQUEST, "store_id 필요");
            return;
        }

        auto catRows = db.executeQuery(
            "SELECT mc.menu_category_id, mc.category_name "
            "FROM menu_categories mc "
            "WHERE mc.restaurant_id=" + std::to_string(storeID) +
            " ORDER BY mc.sort_order, mc.menu_category_id");

        std::string menuQ =
            "SELECT m.menu_id, m.menu_name, m.description, m.price, "
            "       m.image_url, m.is_sold_out, mc.category_name AS sub_category "
            "FROM menus m "
            "JOIN menu_categories mc ON mc.menu_category_id = m.menu_category_id "
            "WHERE mc.restaurant_id=" + std::to_string(storeID);

        if (!subCat.empty() && subCat != "전체")
            menuQ += " AND mc.category_name='" + escapeStr(subCat) + "'";

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
                "SELECT og.option_group_id, og.group_name, "
                "       og.is_essential, og.max_select "
                "FROM option_groups og "
                "WHERE og.menu_id=" + std::to_string(menuID) +
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
                    "SELECT option_item_id, option_name, extra_price "
                    "FROM option_items "
                    "WHERE option_group_id=" + std::to_string(ogID) +
                    " ORDER BY option_item_id");

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
        for (auto& cr : catRows)
            subCats.push_back(cr.at("category_name"));

        json res;
        res["status"]         = Status::SUCCESS;
        res["menus"]          = menus;
        res["sub_categories"] = subCats;
        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_MENU_LIST, res.dump());

    } catch (const std::exception& e) {
        sendError(session, CmdCustomer::REQ_MENU_LIST, Status::SERVER_ERROR, e.what());
    }
}
