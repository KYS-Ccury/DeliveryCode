#include "RiderHandler.h"
#include "ChatHandler.h"

void RiderHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
    // 100번대 공통 프로토콜은 부모에게 위임
    if (protocol >= 100 && protocol <= 105) {
        if (protocol == CmdCommon::REQ_LOGIN) handleLogin(session, jsonBody);
        else if (protocol == CmdCommon::REQ_SIGNUP) handleSignup(session, jsonBody);
        else if (protocol == CmdCommon::REQ_LOGOUT) handleLogout(session, jsonBody);
        return;
    }

    // 400번대 라이더 전용
    switch (protocol) {
        case CmdRider::REQ_DISPATCH_LIST:   handleDispatchList(session, jsonBody); break;
        case CmdRider::REQ_ACCEPT_DISPATCH: handleAcceptDispatch(session, jsonBody); break;
        case CmdRider::REQ_REJECT_DISPATCH: handleRejectDispatch(session, jsonBody); break;
        case CmdRider::REQ_PICKUP_DONE:     handlePickupDone(session, jsonBody); break;
        case CmdRider::REQ_DELIVERY_DONE:   handleDeliveryDone(session, jsonBody); break;
        case CmdRider::REQ_MY_DISPATCHES:   handleMyDispatches(session, jsonBody); break;
        case CmdRider::REQ_WORK_STATUS:     handleWorkStatus(session, jsonBody); break;
        case CmdRider::REQ_SEND_GPS:        handleUpdateGps(session, jsonBody); break;
        
        // 채팅
        case CmdChat::REQ_CREATE_ROOM:
        case CmdChat::REQ_SEND_MSG:
        case CmdChat::REQ_GET_MSGS:
            ChatHandler::process(session, protocol, jsonBody, m_clientType);
            break;
    }
}