//  소켓 설정과 패킷 파싱용 유틸 함수 모음이다.

#pragma once

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include "Packet.h"

//  fd를 non-blocking으로 바꾼다.
inline void setNonBlocking(int fd) {
    //  기존 플래그를 읽는다.
    int flags = fcntl(fd, F_GETFL, 0);

    //  O_NONBLOCK 플래그를 추가한다.
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

//  한 줄 프로토콜 문자열을 Packet으로 변환한다.
//  형식 예시: LOGIN|user1 , ORDER|burger:2 , CHAT|hello
inline Packet parsePacket(int clientFd, const std::string& line) {
    //  기본 패킷을 만든다.
    Packet packet;
    packet.clientFd = clientFd;
    packet.raw = line;

    //  구분자 위치를 찾는다.
    std::size_t pos = line.find('|');

    //  타입 문자열과 payload를 분리한다.
    std::string typeStr = (pos == std::string::npos) ? line : line.substr(0, pos);
    packet.payload = (pos == std::string::npos) ? "" : line.substr(pos + 1);

    //  문자열 타입을 열거형으로 바꾼다.
    if (typeStr == "LOGIN") packet.type = PacketType::AUTH_LOGIN;
    else if (typeStr == "ORDER") packet.type = PacketType::ORDER_CREATE;
    else if (typeStr == "CHAT") packet.type = PacketType::CHAT_SEND;
    else if (typeStr == "RIDER") packet.type = PacketType::RIDER_UPDATE;
    else packet.type = PacketType::UNKNOWN;

    //  파싱된 패킷을 반환한다.
    return packet;
}