#pragma once
#include "Basehandler.h" // 1. 대문자 H로 수정 완료

class CustomerHandler : public BaseHandler {
private:
    CustomerHandler() : BaseHandler(ClientType::CUSTOMER, "CUSTOMER") {}
    ~CustomerHandler() = default;

public:
    static CustomerHandler& getInstance() {
        static CustomerHandler instance;
        return instance;
    }

    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;

    // 외부 호출용 (다른 핸들러에서 고객에게 Push 보낼 때)
    void pushOrderStatus(Session* session, int orderID, int status, const std::string& msg);

protected:

    // 2. 구현부는 Customer_Auth.cpp에 있으므로 {} 를 빼고 ; 로 선언만 합니다.
    void onSignup(Session* session, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout(Session* session, int userId) override;
    void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody) override;

private:
    // 3. 회원 탈퇴 선언 추가
    void handleWithdraw    (Session* s, const std::string& b); // 105

    // 고객 전용 (CmdCustomer 200~210)
    void handleStoreList   (Session* s, const std::string& b); // 200
    void handleMenuList    (Session* s, const std::string& b); // 201
    void handleCreateOrder (Session* s, const std::string& b); // 202
    void handleOrderHistory(Session* s, const std::string& b); // 203
    void handleOrderDetail (Session* s, const std::string& b); // 204
    void handlePayment     (Session* s, const std::string& b); // 205
    void handleWriteReview (Session* s, const std::string& b); // 206
    void handleReviewList  (Session* s, const std::string& b); // 207
    void handleCancelOrder (Session* s, const std::string& b); // 208
};