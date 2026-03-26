#pragma once
#include "Session.h"
#include "Protocol.h"
#include "Struct.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

class BaseHandler {
protected:
    ClientType m_clientType;
    std::string m_roleName;
    std::unordered_map<int, int> m_fdToUserId;
    std::unordered_map<int, int> m_userIdToFd;
    std::mutex m_sessionMtx;

    void registerSession(int fd, int userId);
    void sendError(Session* session, uint16_t protocol, uint16_t statusCode, const std::string& message);
    void sendResponse(Session* session, uint16_t protocol, const nlohmann::json& payload);
   
    // 자식에서 반드시 구현해야 할 훅(Hook)
    virtual void onSignup(Session* session, const nlohmann::json& reqBody) = 0;
    virtual void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) = 0;
    virtual void onLogout(Session* session, int userId) = 0;
    virtual void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody) = 0;

public:
    BaseHandler(ClientType type, const std::string& roleName);
    virtual ~BaseHandler() = default;

    int getUserIdByFd(int fd);
    int getFdByUserId(int userId);
    void unregisterSession(int fd);

    // ★ 공통 로직 (handleGetProfile 추가)
    void handleLogin(Session* session, const std::string& jsonBody);
    void handleSignup(Session* session, const std::string& jsonBody);
    void handleLogout(Session* session, const std::string& jsonBody);
    void handleGetProfile(Session* session, const std::string& jsonBody); 

    virtual void process(Session* session, uint16_t protocol, const std::string& jsonBody) = 0;
};