//  클라이언트 요청을 서버 내부에서 다루기 위한 패킷 구조체이다.
//  현재는 단순 텍스트 프로토콜을 사용하지만 나중에 JSON으로 쉽게 교체할 수 있게 분리한다.
#pragma once

#include <string>

//  패킷 종류를 구분하기 위한 열거형이다.
enum class PacketType {
    UNKNOWN,
    AUTH_LOGIN,
    ORDER_CREATE,
    CHAT_SEND,
    RIDER_UPDATE
};

//  서버 내부에서 사용하는 패킷 구조체이다.
struct Packet {
    //  요청을 보낸 소켓 번호이다.
    int clientFd = -1;

    //  패킷 타입이다.
    PacketType type = PacketType::UNKNOWN;

    //  원본 문자열이다.
    std::string raw;

    //  단순 payload 문자열이다.
    std::string payload;
};