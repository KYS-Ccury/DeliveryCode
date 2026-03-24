#pragma once
#include "Basehandler.h"

class AdminHandler : public BaseHandler {
private:
    AdminHandler() : BaseHandler(ClientType::ADMIN, "ADMIN") {}
    ~AdminHandler() = default;

public:
    static AdminHandler& getInstance() {
        static AdminHandler instance;
        return instance;
    }

    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

protected:
    void onSignup(Session* session, const nlohmann::json& reqBody) override {}
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override {}
    void onLogout(Session* session, int userId) override {}
    void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody) override {}

private:
    // 관리자 전용 로직
    void handleMonitorOrders (Session* s, const std::string& b); // 510
    void handleRiderStatus   (Session* s, const std::string& b); // 511
    void handleForceDispatch (Session* s, const std::string& b); // 512
    void handleForceCancel   (Session* s, const std::string& b); // 513
    void handleSettlementList(Session* s, const std::string& b); // 500
    void handleSettlementConf(Session* s, const std::string& b); // 502
    void handleManageReview  (Session* s, const std::string& b); // 520
};