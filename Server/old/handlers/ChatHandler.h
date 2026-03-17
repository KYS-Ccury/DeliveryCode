//  채팅 관련 요청을 처리하는 핸들러이다.
//  현재는 뼈대이므로 브로드캐스트 대신 수신 확인만 응답한다.

#pragma once

#include "IHandler.h"

class ChatHandler : public IHandler {
public:
    //  채팅 요청을 처리한다.
    void handle(const Packet& packet) override;
};