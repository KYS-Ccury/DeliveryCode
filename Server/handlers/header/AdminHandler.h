#pragma once
#include <string>
#include <cstdint>

class Session;

class AdminHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

    // ChatHandler 연동용
    static void registerSession  (int fd, int adminId);
    static void unregisterSession(int fd);

private:
    static void handleAdminLogin   (Session* s, const std::string& b); // 101
    static void handleGetStats     (Session* s, const std::string& b); // 510
    static void handleRiderStatus  (Session* s, const std::string& b); // 511
    static void handleForceDispatch(Session* s, const std::string& b); // 512
    static void handleForceCancel  (Session* s, const std::string& b); // 513
    static void handleManageReview (Session* s, const std::string& b); // 520
    static void handleSettlement   (Session* s, const std::string& b); // 500
    static void handleBanUser      (Session* s, const std::string& b); // 내부용
};
