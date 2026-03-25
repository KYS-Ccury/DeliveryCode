#pragma once

#include <string>
#include <sstream>
#include "Packet.h"

// =============================
// Packet ??臾몄옄??蹂??
// =============================
inline std::string SerializePacket(const Packet& pkt)
{
    // ?꾩옱??媛꾨떒???띿뒪???꾨줈?좎퐳 ?좎?
    // ?? LOGIN|user1

    std::ostringstream oss;

    switch (pkt.type)
    {
    case PacketType::AUTH_LOGIN:
        oss << "LOGIN|";
        break;

    case PacketType::ORDER_CREATE:
        oss << "ORDER|";
        break;

    case PacketType::CHAT_SEND:
        oss << "CHAT|";
        break;

    case PacketType::RIDER_UPDATE:
        oss << "RIDER|";
        break;

    default:
        oss << "UNKNOWN|";
        break;
    }

    oss << pkt.payload;

    return oss.str();
}

// =============================
// 臾몄옄????Packet 蹂??
// =============================
inline Packet ParsePacket(int clientFd, const std::string& line)
{
    Packet pkt;
    pkt.clientFd = clientFd;
    pkt.raw = line;

    std::size_t pos = line.find('|');

    std::string typeStr = (pos == std::string::npos)
        ? line
        : line.substr(0, pos);

    pkt.payload = (pos == std::string::npos)
        ? ""
        : line.substr(pos + 1);

    if (typeStr == "LOGIN") pkt.type = PacketType::AUTH_LOGIN;
    else if (typeStr == "ORDER") pkt.type = PacketType::ORDER_CREATE;
    else if (typeStr == "CHAT") pkt.type = PacketType::CHAT_SEND;
    else if (typeStr == "RIDER") pkt.type = PacketType::RIDER_UPDATE;
    else pkt.type = PacketType::UNKNOWN;

    return pkt;
}