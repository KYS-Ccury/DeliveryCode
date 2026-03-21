// ============================================================
//  Dispatcher.cpp
//  ★ 변경 사항:
//    - 공통 프로토콜(100~105)도 clientType 별로 분기
//      → 라이더가 보낸 로그인/회원가입/프로필 요청은 RiderHandler에서 처리
//    - 기존 구조 완전 유지, 공통 분기만 추가
// ============================================================
#include "Dispatcher.h"
#include "CustomerHandler.h"
#include "RiderHandler.h"
#include "AdminHandler.h"
#include "OwnerHandler.h"
#include "Session.h"
#include "Types.h"
#include <iostream>

void Dispatcher::dispatch(Session* session, const PacketHeader& header, const std::string& jsonBody) {
    ClientType cType   = static_cast<ClientType>(header.clientType);
    uint16_t   protocol = header.protocol;

    // ── 공통 프로토콜(100번대)은 clientType 기준으로 각 핸들러에 위임 ──
    // 각 핸들러의 process() switch 에 CmdCommon 케이스가 포함되어 있음
    bool isCommon = (protocol >= 100 && protocol <= 105);

    switch (cType) {
        case ClientType::CUSTOMER:
            CustomerHandler::process(session, protocol, jsonBody);
            break;

        case ClientType::OWNER:
            OwnerHandler::process(session, protocol, jsonBody);
            break;

        case ClientType::RIDER:
            // 공통(로그인·로그아웃·프로필) + 라이더 전용(400번대) 모두 처리
            RiderHandler::process(session, protocol, jsonBody);
            break;

        case ClientType::ADMIN:
            AdminHandler::process(session, protocol, jsonBody);
            break;

        default:
            std::cerr << "[Dispatcher] 알 수 없는 ClientType: "
                      << static_cast<int>(cType)
                      << " / protocol=" << protocol << std::endl;
            break;
    }
}
