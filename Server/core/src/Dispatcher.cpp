#include "Dispatcher.h"
#include "CustomerHandler.h"
#include "RiderHandler.h"
#include "AdminHandler.h"
// #include "OwnerHandler.h"
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

    try {
        switch (cType) {
            case ClientType::CUSTOMER:
                // ★ 수정: responseJson = 삭제 (내부에서 알아서 send 함)
                CustomerHandler::getInstance().process(session, protocol, jsonBody); 
                break;
                
            case ClientType::OWNER:
                // OwnerHandler::getInstance().process(session, protocol, jsonBody);
                break;

            case ClientType::RIDER:
                // ★ 수정: responseJson = 삭제
                RiderHandler::getInstance().process(session, protocol, jsonBody);
                break;
            
            case ClientType::ADMIN:
                // AdminHandler::getInstance().process(session, protocol, jsonBody);
                break;

            default:
                std::cerr << "[Dispatcher] 정의되지 않은 ClientType: " 
                          << static_cast<int>(cType) << " (Protocol: " << protocol << ")" << std::endl;
                break;
        } // switch 닫기
    } catch (const std::exception& e) { // ★ 수정: 빼먹으셨던 catch 구문 추가
        std::cerr << "[Dispatcher] 예외 발생: " << e.what() << std::endl;
    }
}