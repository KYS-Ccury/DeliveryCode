//  라이더 관련 요청을 처리하는 핸들러이다.

#pragma once

#include "IHandler.h"

class RiderHandler : public IHandler {
public:
    //  라이더 요청을 처리한다.
    void handle(const Packet& packet) override;
};