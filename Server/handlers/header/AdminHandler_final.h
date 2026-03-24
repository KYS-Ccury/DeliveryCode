#pragma once
#include <string>
#include <cstdint>

class Session;

class AdminHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

    // EpollServer 세션 등록/해제 시 호출
    static void registerSession  (int fd, int adminId);
    static void unregisterSession(int fd);

private:
    static void handleAdminLogin    (Session* s, const std::string& b); // 101
    static void handleMonitorOrders (Session* s, const std::string& b); // 510
    static void handleRiderStatus   (Session* s, const std::string& b); // 511
    static void handleForceDispatch (Session* s, const std::string& b); // 512
    static void handleForceCancel   (Session* s, const std::string& b); // 513
    static void handleSettlementList(Session* s, const std::string& b); // 500
    static void handleSettlementConf(Session* s, const std::string& b); // 502
    static void handleManageReview  (Session* s, const std::string& b); // 520
    // handleGetStats, handleBanUser는 handleMonitorOrders/handleManageReview로 통합
};
