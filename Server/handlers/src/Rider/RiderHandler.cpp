// #include "RiderHandler.h"
// #include "ChatUtil.h"
// #include <iostream>

// void RiderHandler::process(Session* session, uint16_t protocol, const std::string& jsonBody) {
//     // 1. 100번대 공통 프로토콜 처리
//     if (protocol >= 100 && protocol <= 199) { 
//         switch (protocol) {
//             case CmdCommon::REQ_LOGIN:       handleLogin(session, jsonBody); break;
//             case CmdCommon::REQ_SIGNUP:      handleSignup(session, jsonBody); break;
//             case CmdCommon::REQ_LOGOUT:      handleLogout(session, jsonBody); break;
//             case CmdCommon::REQ_GET_PROFILE: handleGetProfile(session, jsonBody); break; // ★ 추가
//             default:
//                 break;
//         }
//         return; 
//     }

//     // 2. 400번대 라이더 전용 및 채팅 프로토콜 처리
//     switch (protocol) {
//         // 라이더 기능
//         case CmdRider::REQ_DISPATCH_LIST:   handleDispatchList(session, jsonBody); break;
//         case CmdRider::REQ_ACCEPT_DISPATCH: handleAcceptDispatch(session, jsonBody); break;
//         case CmdRider::REQ_REJECT_DISPATCH: handleRejectDispatch(session, jsonBody); break;
//         case CmdRider::REQ_PICKUP_DONE:     handlePickupDone(session, jsonBody); break;
//         case CmdRider::REQ_DELIVERY_DONE:   handleDeliveryDone(session, jsonBody); break;
//         case CmdRider::REQ_MY_DISPATCHES:   handleMyDispatches(session, jsonBody); break;
//         case CmdRider::REQ_WORK_STATUS:     handleWorkStatus(session, jsonBody); break;
//         case CmdRider::REQ_SEND_GPS:        handleUpdateGps(session, jsonBody); break;
        
//         // 채팅 기능
//         // case CmdChat::REQ_CREATE_ROOM:
//         // case CmdChat::REQ_SEND_MSG:
//         // case CmdChat::REQ_GET_MSGS:
//         //     ChatHandler::process(session, protocol, jsonBody, m_clientType); 
//         //     break;

//         default:
//             std::cerr << "[RiderHandler] Unknown protocol: " << protocol << std::endl;
//             sendError(session, protocol, 400, "Unknown Rider Protocol");
//             break;
//     }
// }