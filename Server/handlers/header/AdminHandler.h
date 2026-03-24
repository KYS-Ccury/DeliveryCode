#pragma once
#include <string>
#include <cstdint>

class Session;

class AdminHandler {
public:
    static void process(Session* session, uint16_t protocol,
                        const std::string& jsonBody);

    static void registerSession  (int fd, int adminId);
    static void unregisterSession(int fd);

private:
    static void handleAdminLogin    (Session* s, const std::string& b); // 503
    static void handleGetStats      (Session* s, const std::string& b); // 500
    static void handleBanUser       (Session* s, const std::string& b); // 501
    static void handleForceCancel   (Session* s, const std::string& b); // 502
    static void handleForceDispatch (Session* s, const std::string& b); // 512
    static void handleRiderStatus   (Session* s, const std::string& b); // 513
    static void handleManageReview  (Session* s, const std::string& b); // 514
    static void handleSettlement    (Session* s, const std::string& b); // 515
    static void handleSettlementList(Session* s, const std::string& b); // 516
    static void handleSettlementConf(Session* s, const std::string& b); // 517
    static void handleMonitorOrders (Session* s, const std::string& b); // 518
};
