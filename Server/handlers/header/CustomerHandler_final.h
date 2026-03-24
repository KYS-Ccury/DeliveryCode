#pragma once
#include <string>
#include <cstdint>

class Session;

class CustomerHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

    // NTF_ORDER_STATUS (210) Push – OwnerHandler/RiderHandler에서 호출
    static void pushOrderStatus(Session* session, int orderID, int status, const std::string& msg);

private:
    // 공통 인증 (CmdCommon 100~105)
    static void handleSignup     (Session* s, const std::string& b); // 100
    static void handleLogin      (Session* s, const std::string& b); // 101
    static void handleLogout     (Session* s, const std::string& b); // 102
    static void handleGetProfile (Session* s, const std::string& b); // 104
    static void handleWithdraw   (Session* s, const std::string& b); // 105

    // 고객 전용 (CmdCustomer 200~210)
    static void handleStoreList   (Session* s, const std::string& b); // 200
    static void handleMenuList    (Session* s, const std::string& b); // 201
    static void handleCreateOrder (Session* s, const std::string& b); // 202
    static void handleOrderHistory(Session* s, const std::string& b); // 203
    static void handleOrderDetail (Session* s, const std::string& b); // 204
    static void handlePayment     (Session* s, const std::string& b); // 205
    static void handleWriteReview (Session* s, const std::string& b); // 206
    static void handleReviewList  (Session* s, const std::string& b); // 207
    static void handleCancelOrder (Session* s, const std::string& b); // 208
};
