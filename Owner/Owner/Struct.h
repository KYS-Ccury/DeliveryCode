#pragma once
#include <stdint.h>

#pragma pack(push, 1) // 1바이트 단위 정렬 (패딩 제거)
struct PacketHeader {
    uint8_t  type;       // 1바이트: ClientType (OWNER = 2)
    uint16_t protocol;   // 2바이트: Cmd ID (100, 101 등)
    uint32_t len;        // 4바이트: 바디(JSON)의 길이
};
#pragma pack(pop)