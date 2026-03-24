// ============================================================
//  Dispatcher.cpp
//  ★ 최종 수정: 모든 핸들러를 싱글톤(getInstance) 호출 방식으로 통일
// ============================================================
#include "Dispatcher.h"
#include "CustomerHandler.h"
#include "RiderHandler.h"
#include "AdminHandler.h"
#include "OwnerHandler.h"
#include "Session.h"
#include "Struct.h"   // Packet.h에서 이름 변경됨
#include "Protocol.h"
#include <iostream>

void Dispatcher::dispatch(Session* session, const PacketHeader& header, const std::string& jsonBody) {
    if (!session) return;

    // header에서 clientType과 protocol 추출
    ClientType cType   = static_cast<ClientType>(header.clientType);
    uint16_t   protocol = header.protocol;

    // 로그 (디버깅용)
    // std::cout << "[Dispatcher] Type: " << static_cast<int>(cType) << ", Protocol: " << protocol << std::endl;

    switch (cType) {
        case ClientType::CUSTOMER:
            // 모든 핸들러를 getInstance().process()로 통일합니다.
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
            std::cerr << "[Dispatcher] 정의되지 않은 ClientType: " 
                      << static_cast<int>(cType) << " (Protocol: " << protocol << ")" << std::endl;
            break;
    }
}