// Common/Packet.h

#pragma once

#include <string>

// ?⑦궥 醫낅쪟
enum class PacketType {
    UNKNOWN = 0,

    AUTH_LOGIN = 100,
    ORDER_CREATE = 200,
    CHAT_SEND = 300,
    RIDER_UPDATE = 400
};

// ?쒕쾭 ?대? ?⑦궥 援ъ“
struct Packet {
    int clientFd = -1;          // ?뚯폆 FD

    PacketType type = PacketType::UNKNOWN;

    std::string raw;            // ?먮낯 (?붾쾭源낆슜)
    std::string payload;        // ?꾩옱 ?ъ슜 ?곗씠??

    // 異붽? (?섏쨷 JSON??
    std::string jsonBody;       // JSON 臾몄옄??(?뺤옣??
};