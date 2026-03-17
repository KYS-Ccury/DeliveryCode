//  모든 핸들러가 따라야 하는 공통 인터페이스이다.

#pragma once

#include "../server/Packet.h"

class IHandler {
public:
    //  가상 소멸자이다.
    virtual ~IHandler() = default;

    //  패킷을 처리하는 인터페이스이다.
    virtual void handle(const Packet& packet) = 0;
};