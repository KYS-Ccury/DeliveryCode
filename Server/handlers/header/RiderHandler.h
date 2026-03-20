#pragma once
#include <string>
#include <cstdint>

class Session; // 전방 선언

class RiderHandler {
public:
    // 메인 디스패치 함수
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

private:
    // 실제 기능 구현 함수 (기능 개발 중)
    static void handleUpdateGps(Session* session, const std::string& jsonBody);
    static void handlePickupDone(Session* session, const std::string& jsonBody);
};