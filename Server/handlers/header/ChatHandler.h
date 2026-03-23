#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include "Types.h"

class Session;

// ============================================================
//  ChatHandler
//  라이더 ↔ 관리자 실시간 채팅 처리
// ============================================================
class ChatHandler {
public:
    // 메인 디스패치 (RiderHandler/AdminHandler에서 호출)
    static void process(Session* session, uint16_t protocol,
                        const std::string& jsonBody,
                        ClientType clientType);

    // 관리자 세션 관리
    static void registerAdmin  (int fd, int adminId);
    static void unregisterAdmin(int fd);
    static int  getAdminIdByFd (int fd);
    static int  getAdminFd();   // 현재 접속 관리자 fd 반환

private:
    static void handleCreateRoom(Session* session, const std::string& body, ClientType ct);
    static void handleSendMsg   (Session* session, const std::string& body, ClientType ct);
    static void handleGetMsgs   (Session* session, const std::string& body, ClientType ct);

    static std::unordered_map<int, int> s_fdToAdmin;   // fd → adminId
    static std::unordered_map<int, int> s_adminToFd;   // adminId → fd
    static std::mutex                   s_adminMtx;
};
