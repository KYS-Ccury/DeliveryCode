#pragma once
#include "Basehandler.h" // 상속을 위해 포함

class OwnerHandler : public BaseHandler {
private:
    OwnerHandler() : BaseHandler(ClientType::OWNER, "OWNER") {}
    ~OwnerHandler() = default;

public:
    static OwnerHandler& getInstance() {
        static OwnerHandler instance;
        return instance;
    }

    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

protected:
    // BaseHandler의 훅 구현 (필요 시)
    void onSignup(Session* session, const nlohmann::json& reqBody) override {}
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override {}
    void onLogout(Session* session, int userId) override {}
    void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody) override {}

private:
    // 사장님 전용 로직
    void handleStoreInfo   (Session* s, const std::string& body); // 300
    void handleUpdateStore (Session* s, const std::string& body); // 301
    void handleAddMenu     (Session* s, const std::string& body); // 302
    void handleSoldOut     (Session* s, const std::string& body); // 303
    void handleOrderList   (Session* s, const std::string& body); // 304
    void handleAcceptOrder (Session* s, const std::string& body); // 305
    void handleRejectOrder (Session* s, const std::string& body); // 306
    void handleCookingDone (Session* s, const std::string& body); // 307
    void handleSalesStats  (Session* s, const std::string& body); // 308
    void handleChangeStatus(Session* s, const std::string& body); // 309
};