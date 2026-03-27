#pragma once
#include "Basehandler.h"

class OwnerHandler : public BaseHandler {
private:
    // ClientType::OWNER (2)와 역할 이름 "OWNER"를 부모에게 전달
    OwnerHandler() : BaseHandler(ClientType::OWNER, "OWNER") {}

public:
    static OwnerHandler& getInstance() {
        static OwnerHandler instance;
        return instance;
    }

    OwnerHandler(const OwnerHandler&) = delete;
    OwnerHandler& operator=(const OwnerHandler&) = delete;
    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

protected:
    // 부모(BaseHandler)가 호출해 줄 훅(Hook) 함수들
    void onSignup(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout(Session* session, int userId) override;
    void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody) override;

private:
    // 주문 관련
    void handleOrderList(Session* session, const std::string& jsonBody);
    void handleAcceptOrder(Session* session, const std::string& jsonBody);
    void handleRejectOrder(Session* session, const std::string& jsonBody);
    
    // 🚨 [추가] 매장(메뉴) 관리 기능 선언
    void handleMenuList(Session* session, const std::string& jsonBody);
    void handleAddMenu(Session* session, const std::string& jsonBody);
    void handleUpdateMenu(Session* session, const std::string& jsonBody);
    void handleDeleteMenu(Session* session, const std::string& jsonBody);
    void handleSalesStats(Session* session, const std::string& jsonBody);
    void handleGetSettings(Session* session, const std::string& jsonBody);
    void handleUpdateSettings(Session* session, const std::string& jsonBody);
    void handleUpdateStatus(Session* session, const std::string& jsonBody);
    void handleCreateRoom(Session* session, const std::string& jsonBody);
    void handleSendMsg(Session* session, const std::string& jsonBody);
    void handleGetMsgs(Session* session, const std::string& jsonBody);
};