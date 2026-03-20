#pragma once
#include <string>
#include <cstdint>

class Session; // 전방 선언

class CustomerHandler {
public:
    // 메인 디스패치 함수
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

private:
    // 실제 기능 구현 함수 (내부 호출용)
    static void handleStoreList(Session* session, const std::string& jsonBody);
    static void handleCreateOrder(Session* session, const std::string& jsonBody);
    static void handleOrderHistory(Session* session, const std::string& jsonBody);
    // 필요 시 추가: handleCancelOrder, handleWriteReview 등
};