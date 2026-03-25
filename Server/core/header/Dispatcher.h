#pragma once

#include <string>
#include "Struct.h"

// 순환 참조(Circular Dependency) 방지를 위해 헤더 포함 대신 전방 선언만 수행
class Session;

class Dispatcher {
public:
    // 스레드 풀 내부에서 객체 생성 없이 바로 호출할 수 있도록 static으로 선언합니다.
    static void dispatch(Session* session, const PacketHeader& header, const std::string& jsonBody);
};