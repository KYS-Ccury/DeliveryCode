#include "Dispatcher.h"
#include "CustomerHandler.h"
#include "RiderHandler.h"
#include "AdminHandler.h"
#include "OwnerHandler.h"
#include "Session.h"
#include "Struct.h" 
#include "Protocol.h"
#include <iostream>

void Dispatcher::dispatch(Session* session, const PacketHeader& header, const std::string& jsonBody) {
    if (!session) return;

    ClientType cType    = static_cast<ClientType>(header.clientType);
    uint16_t   protocol = header.protocol;

    // 디버그 로그 (연결 문제 추적용)
    std::cout << "[Dispatcher] fd=" << session->getFd()
              << " type=" << static_cast<int>(cType)
              << " protocol=" << protocol
              << " bodyLen=" << jsonBody.size() << std::endl;

    try {
        switch (cType) {
            case ClientType::CUSTOMER:
                CustomerHandler::getInstance().process(session, protocol, jsonBody);
                break;

            case ClientType::OWNER:
                OwnerHandler::getInstance().process(session, protocol, jsonBody);
                break;

            case ClientType::RIDER:
                RiderHandler::getInstance().process(session, protocol, jsonBody);
                break;

            case ClientType::ADMIN:
                AdminHandler::getInstance().process(session, protocol, jsonBody);
                break;

            default:
                std::cerr << "[Dispatcher] 알 수 없는 ClientType: "
                          << static_cast<int>(cType)
                          << " protocol=" << protocol << std::endl;
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Dispatcher] 예외: " << e.what()
                  << " type=" << static_cast<int>(cType)
                  << " protocol=" << protocol << std::endl;
    }
}