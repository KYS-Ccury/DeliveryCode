// Common/Packet.h

#pragma once

#include <string>

// 패킷 종류
enum class PacketType {
    UNKNOWN = 0,

    AUTH_LOGIN = 100,
    ORDER_CREATE = 200,
    CHAT_SEND = 300,
    RIDER_UPDATE = 400
};

// 서버 내부 패킷 구조
struct Packet {
    int clientFd = -1;          // 소켓 FD

    PacketType type = PacketType::UNKNOWN;

    std::string raw;            // 원본 (디버깅용)
    std::string payload;        // 현재 사용 데이터

    // 추가 (나중 JSON용)
    std::string jsonBody;       // JSON 문자열 (확장용)
};