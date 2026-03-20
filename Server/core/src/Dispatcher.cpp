#include "Dispatcher.h"
#include "CustomerHandler.h" // 반드시 추가
#include "RiderHandler.h"    // 반드시 추가
#include "AdminHandler.h"
#include "OwnerHandler.h"
#include "Session.h" // Session을 사용하므로 필수
#include "Types.h"   // CMD_CUSTOMER_ORDER 등이 정의된 헤더
#include <iostream>

void Dispatcher::dispatch(Session* session, const PacketHeader& header, const std::string& jsonBody) {
    // 헤더에서 클라이언트 타입과 프로토콜 추출
    ClientType cType = static_cast<ClientType>(header.clientType);
    uint16_t protocol = header.protocol;

    switch (cType) {
        case ClientType::CUSTOMER:
            // 고객 관련 모든 요청은 CustomerHandler가 담당
            CustomerHandler::process(session, protocol, jsonBody);
            break;

        case ClientType::OWNER:
            OwnerHandler::process(session, protocol, jsonBody);
            break;

        case ClientType::RIDER:
            RiderHandler::process(session, protocol, jsonBody);
            break;

        case ClientType::ADMIN:
            AdminHandler::process(session, protocol, jsonBody);
            break;

        default:
            std::cerr << "Unknown Client Type: " << static_cast<int>(cType) << std::endl;
            break;
    }
}