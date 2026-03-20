#include "Dispatcher.h"
#include "CustomerHandler.h"
#include "OwnerHandler.h"
// ...

void Dispatcher::dispatch(Session* session, const PacketHeader& header, const std::string& jsonBody) {
    switch (header.clientType) {
        case 1: // 고객
            CustomerHandler::process(session, header.protocol, jsonBody);
            break;
        case 2: // 사장님
            OwnerHandler::process(session, header.protocol, jsonBody);
            break;
        case 3: // 라이더
            RiderHandler::process(session, header.protocol, jsonBody);
            break;
        case 4: // 관리자
            AdminHandler::process(session, header.protocol, jsonBody);
            break;
        default:
            // 에러: 알 수 없는 클라이언트 타입
            break;
    }
}