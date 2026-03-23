#pragma once
#include <string>
#include <cstdint>

class Session;

class AdminHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

    // 관리자 세션 등록/해제 (EpollServer::closeConnection에서 호출)
    static void registerSession  (int fd, int adminId);
    static void unregisterSession(int fd);

private:
    static void handleGetStats   (Session* session, const std::string& jsonBody);
    static void handleBanUser    (Session* session, const std::string& jsonBody);
    static void handleForceCancel(Session* session, const std::string& jsonBody);
    static void handleAdminLogin (Session* session, const std::string& jsonBody);
};
