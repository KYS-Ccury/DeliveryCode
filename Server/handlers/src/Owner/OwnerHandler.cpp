#include "OwnerHandler.h"
#include <iostream>

using json = nlohmann::json;

void OwnerHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    switch (protocol) {
        case CmdCommon::REQ_LOGIN: handleLogin(session, jsonBody); break;
        case CmdCommon::REQ_SIGNUP: handleSignup(session, jsonBody); break;
        case CmdCommon::REQ_LOGOUT: handleLogout(session, jsonBody); break;
        case CmdCommon::REQ_GET_PROFILE: handleGetProfile(session, jsonBody); break;
        case CmdOwner::REQ_ORDER_LIST:  handleOrderList(session, jsonBody); break;
        case CmdOwner::REQ_ACCEPT_ORDER: handleAcceptOrder(session, jsonBody); break;
        case CmdOwner::REQ_REJECT_ORDER:handleRejectOrder(session, jsonBody); break;
        case CmdOwner::REQ_MENU_LIST:   handleMenuList(session, jsonBody); break;
        case CmdOwner::REQ_ADD_MENU:    handleAddMenu(session, jsonBody); break;
        case CmdOwner::REQ_UPDATE_MENU: handleUpdateMenu(session, jsonBody); break;
        case CmdOwner::REQ_DEL_MENU:    handleDeleteMenu(session, jsonBody); break;
        case CmdOwner::REQ_SALES_STATS: handleSalesStats(session, jsonBody); break;
        case CmdOwner::REQ_GET_SETTINGS:    handleGetSettings(session, jsonBody); break;
        case CmdOwner::REQ_UPDATE_SETTINGS: handleUpdateSettings(session, jsonBody); break;
        case CmdOwner::REQ_UPDATE_STATUS: handleUpdateStatus(session, jsonBody); break;
        case CmdChat::REQ_CREATE_ROOM: handleCreateRoom(session, jsonBody); break;
        case CmdChat::REQ_SEND_MSG:    handleSendMsg(session, jsonBody); break;
        case CmdChat::REQ_GET_MSGS:    handleGetMsgs(session, jsonBody); break;
        
        default:
            std::cerr << "[OwnerHandler] 알 수 없는 프로토콜: " << protocol << "\n";
            break;
    }
}