#include "OwnerDB.h"
#include "Protocol.h"
#include <iostream>

using json = nlohmann::json;

json OwnerDB::process(uint16_t dbProtocol, const json& reqJson) {
    json res;
    try {
        switch (dbProtocol) {
            case CmdDBOwner::REQ_DB_STORE_INFO: { 
                int ownerId = reqJson.value("owner_id", -1);
                return getInstance().getStoreInfoByOwnerId(ownerId);
            }
            case CmdDBOwner::REQ_DB_UPDATE_STORE: {
                if (reqJson.value("action", "") == "CREATE") {
                    int ownerId = reqJson.value("owner_id", -1);
                    std::string storeName = reqJson.value("store_name", "");
                    bool ok = getInstance().insertOwnerProfile(ownerId, storeName);
                    res["status"] = ok ? Status::SUCCESS : Status::SERVER_ERROR;
                    return res;
                }
                break;
            }
            case CmdDBOwner::REQ_DB_ORDER_LIST: return getInstance().getOrderList(reqJson);
            case CmdDBOwner::REQ_DB_ACCEPT_ORDER: return getInstance().acceptOrder(reqJson);
            case CmdDBOwner::REQ_DB_REJECT_ORDER: return getInstance().rejectOrder(reqJson);
            case CmdDBOwner::REQ_DB_ADD_MENU: return getInstance().addMenu(reqJson);
            case CmdDBOwner::REQ_DB_UPDATE_MENU: return getInstance().updateMenu(reqJson);
            case CmdDBOwner::REQ_DB_DEL_MENU: return getInstance().deleteMenu(reqJson);
            case CmdDBOwner::REQ_DB_MENU_LIST: { int ownerId = reqJson.value("owner_id", -1); return getInstance().getMenuList(ownerId); }
            case CmdDBOwner::REQ_DB_SALES_STATS: return getInstance().getSalesStats(reqJson);
            case CmdDBOwner::REQ_DB_GET_SETTINGS:    return getInstance().getSettings(reqJson);
            case CmdDBOwner::REQ_DB_UPDATE_SETTINGS: return getInstance().updateSettings(reqJson);
            case CmdDBOwner::REQ_DB_UPDATE_STATUS: return getInstance().updateStoreStatus(reqJson);

            default:
                res["status"] = Status::SERVER_ERROR;
                res["message"] = "OwnerDB Unknown Protocol";
                return res;
        }
    } catch (const std::exception& e) {
        std::cerr << "[OwnerDB] Exception: " << e.what() << std::endl;
        res["status"] = Status::SERVER_ERROR;
        return res;
    }
    return res;
}