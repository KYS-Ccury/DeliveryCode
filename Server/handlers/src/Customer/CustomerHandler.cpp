#include "CustomerHandler.h"
#include "Session.h"
#include "Protocol.h"
#include "ChatUtil.h"
#include <iostream>

void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& body) {
    // 1. 100번대 공통 프로토콜 처리 (switch문으로 통일)
    if (protocol >= 100 && protocol <= 199) {
        switch (protocol) {
            case CmdCommon::REQ_SIGNUP:      handleSignup(session, body); break;
            case CmdCommon::REQ_LOGIN:       handleLogin(session, body); break;
            case CmdCommon::REQ_LOGOUT:      handleLogout(session, body); break;
            case CmdCommon::REQ_GET_PROFILE: handleGetProfile(session, body); break;
            case CmdCommon::REQ_WITHDRAW:    handleWithdraw(session, body); break; // 105번 탈퇴
            default:
                // 필요시 공통 프로토콜 에러 처리
                break;
        }
        return; // 공통 프로토콜 처리 후 종료
    }

    // 2. 200번대 고객 전용 프로토콜
    switch (protocol) {
        case CmdCustomer::REQ_STORE_LIST:   handleStoreList   (session, body); break;
        case CmdCustomer::REQ_MENU_LIST:    handleMenuList    (session, body); break;
        case CmdCustomer::REQ_CREATE_ORDER: handleCreateOrder (session, body); break;
        case CmdCustomer::REQ_ORDER_HISTORY:handleOrderHistory(session, body); break;
        case CmdCustomer::REQ_ORDER_DETAIL: handleOrderDetail (session, body); break;
        case CmdCustomer::REQ_PAYMENT:      handlePayment     (session, body); break;
        case CmdCustomer::REQ_WRITE_REVIEW: handleWriteReview (session, body); break;
        case CmdCustomer::REQ_REVIEW_LIST:  handleReviewList  (session, body); break;
        case CmdCustomer::REQ_CANCEL_ORDER: handleCancelOrder (session, body); break;
        // case CmdChat::REQ_CREATE_ROOM:
        // case CmdChat::REQ_SEND_MSG:
        // case CmdChat::REQ_GET_MSGS:
        //     ChatDispatcher::getInstance().process(session, protocol, body, m_clientType); 
        //     break;
        default:
            std::cerr << "[CustomerHandler] 알 수 없는 프로토콜: " << protocol << "\n";
            sendError(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
            break;
    }
} // ★ 누락되었던 닫는 중괄호 추가

// ─────────────────────────────────────────────────
// NTF_ORDER_STATUS (210): 서버 → 고객 Push
// ─────────────────────────────────────────────────
void CustomerHandler::pushOrderStatus(Session* session, int orderID, int status, const std::string& msg) {
    nlohmann::json ntf;
    ntf["order_id"] = orderID;
    ntf["status"]   = status;
    ntf["message"]  = msg;
    session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
}