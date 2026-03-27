#pragma once
#include <stdint.h>

// 1바이트 정렬 강제 (양쪽 플랫폼 모두 설정해야 함)
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  clientType;    // 1바이트: 디스패치 대상 (1:고객, 2:사장, 3:라이더, 4:관리자)
    uint16_t protocol;      // 2바이트: 수행할 명령 (예: 로그인, 주문 등)
    uint32_t bodyLength;    // 4바이트: 가변 JSON 바디 길이
};
#pragma pack(pop)