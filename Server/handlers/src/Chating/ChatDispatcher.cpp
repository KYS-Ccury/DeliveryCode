// // ============================================================
// //  ChatDispatcher.cpp [채팅 중재자/디스패처]
// // ============================================================
// #include "ChatDispatcher.h"
// #include "CustomerChatHandler.h"
// #include "OwnerChatHandler.h"
// #include "RiderChatHandler.h"
// #include "AdminChatHandler.h"
// #include <iostream>

// ChatDispatcher& ChatDispatcher::getInstance() {
//     static ChatDispatcher instance;
//     return instance;
// }

// void ChatDispatcher::process(Session* session, uint16_t protocol,
//                              const std::string& jsonBody,
//                              ClientType clientType) {
//     switch (clientType) {
//         case ClientType::CUSTOMER:
//             CustomerChatHandler::getInstance().process(session, protocol, jsonBody);
//             break;
            
//         case ClientType::OWNER:
//             OwnerChatHandler::getInstance().process(session, protocol, jsonBody);
//             break;
            
//         case ClientType::RIDER:
//             RiderChatHandler::getInstance().process(session, protocol, jsonBody);
//             break;
            
//         case ClientType::ADMIN:
//             AdminChatHandler::getInstance().process(session, protocol, jsonBody);
//             break;
            
//         default:
//             std::cerr << "[ChatDispatcher] 알 수 없는 ClientType: " 
//                       << static_cast<int>(clientType) << std::endl;
//             break;
//     }
// }