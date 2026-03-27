#include "CustomerHandler.h"
#include "Session.h"
#include "Protocol.h"
#include "ChatUtil.h"
#include <iostream>

using json = nlohmann::json;

void CustomerHandler::process(Session* session, uint16_t protocol, const std::string& body) {

    // ── 100번대: 공통 인증 프로토콜 ─────────────────────────
    if (protocol >= 100 && protocol <= 199) {
        switch (protocol) {
            case CmdCommon::REQ_SIGNUP:   handleSignup(session, body);  break;
            case CmdCommon::REQ_LOGIN:    handleLogin(session, body);   break;
            case CmdCommon::REQ_LOGOUT:   handleLogout(session, body);  break;
            case CmdCommon::REQ_WITHDRAW: handleWithdraw(session, body); break;

            case CmdCommon::REQ_GET_PROFILE: {
                try {
                    json req = json::parse(body.empty() ? "{}" : body);
                    std::string reqType = req.value("request_type", "");
                    if (reqType == "get_cards"       ||
                        reqType == "add_card"        ||
                        reqType == "delete_card"     ||
                        reqType == "set_default_card")
                    {
                        int uid = getUserIdByFd(session->getFd());
                        if (uid <= 0) {
                            sendError(session, CmdCommon::REQ_GET_PROFILE,
                                      Status::UNAUTHORIZED, "로그인이 필요합니다.");
                            break;
                        }
                        onGetProfile(session, uid, req);
                        break;
                    }
                } catch (...) {}
                handleGetProfile(session, body);
                break;
            }
            default: break;
        }
        return;
    }

    // ── 200번대: 고객 전용 프로토콜 ─────────────────────────
    if (protocol >= 200 && protocol <= 299) {
        switch (protocol) {
            case CmdCustomer::REQ_STORE_LIST:    handleStoreList   (session, body); break;
            case CmdCustomer::REQ_MENU_LIST:     handleMenuList    (session, body); break;
            case CmdCustomer::REQ_CREATE_ORDER:  handleCreateOrder (session, body); break;
            case CmdCustomer::REQ_ORDER_HISTORY: handleOrderHistory(session, body); break;
            case CmdCustomer::REQ_ORDER_DETAIL:  handleOrderDetail (session, body); break;
            case CmdCustomer::REQ_PAYMENT:       handlePayment     (session, body); break;
            case CmdCustomer::REQ_WRITE_REVIEW:  handleWriteReview (session, body); break;
            case CmdCustomer::REQ_REVIEW_LIST:   handleReviewList  (session, body); break;
            case CmdCustomer::REQ_CANCEL_ORDER:    handleCancelOrder   (session, body); break;
            case CmdCustomer::REQ_MY_POINT:        handleMyPoint       (session, body); break;
            case CmdCustomer::REQ_CHANGE_PASSWORD: handleChangePassword(session, body); break;
            case CmdCustomer::REQ_MY_INFO:         handleMyInfo        (session, body); break;
            // ★ 주소 관련 (신규)
            case CmdCustomer::REQ_GET_ADDRESSES:    handleGetAddresses     (session, body); break;
            case CmdCustomer::REQ_SAVE_ADDRESS:     handleSaveAddress      (session, body); break;
            case CmdCustomer::REQ_DELETE_ADDRESS:   handleDeleteAddress    (session, body); break;
            case CmdCustomer::REQ_DEFAULT_ADDRESS:  handleSetDefaultAddress(session, body); break;
            case CmdCustomer::REQ_GET_IMAGE:        handleGetImage         (session, body); break; // 217
            default:
                std::cerr << "[CustomerHandler] 알 수 없는 200번대 프로토콜: " << protocol << "\n";
                sendError(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
                break;
        }
        return;
    }

    // ── 600번대: 채팅 프로토콜 ───────────────────────────────
    // if (protocol >= 600 && protocol <= 699) {
    //     switch (protocol) {
    //         case CmdChat::REQ_CREATE_ROOM: handleChatCreateRoom(session, body); break;
    //         case CmdChat::REQ_SEND_MSG:    handleChatSendMsg   (session, body); break;
    //         case CmdChat::REQ_GET_MSGS:    handleChatGetMsgs   (session, body); break;
    //         default:
    //             std::cerr << "[CustomerHandler] 알 수 없는 600번대 프로토콜: " << protocol << "\n";
    //             sendError(session, protocol, Status::BAD_REQUEST, "Unknown chat protocol");
    //             break;
    //     }
    //     return;
    // }

    std::cerr << "[CustomerHandler] 미처리 프로토콜: " << protocol << "\n";
    sendError(session, protocol, Status::BAD_REQUEST, "Unknown protocol");
}

// ─────────────────────────────────────────────────
// NTF_ORDER_STATUS (210): 서버 → 고객 Push
// ─────────────────────────────────────────────────
void CustomerHandler::pushOrderStatus(Session* session, int orderID,
                                       int status, const std::string& msg) {
    nlohmann::json ntf;
    ntf["order_id"] = orderID;
    ntf["status"]   = status;
    ntf["message"]  = msg;
    session->sendPacket(static_cast<uint8_t>(m_clientType),
                        CmdCustomer::NTF_ORDER_STATUS, ntf.dump());
}
