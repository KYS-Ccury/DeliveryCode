#pragma once

#include "Basehandler.h"

class CustomerHandler : public BaseHandler {
public:
    static CustomerHandler& getInstance() {
        static CustomerHandler instance;
        return instance;
    }

    // ── BaseHandler 순수 가상함수 override ───────────────────
    void onSignup      (Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout      (Session* session, int userId) override;
    void onGetProfile  (Session* session, int userId, const nlohmann::json& reqBody) override;

    // Dispatcher 진입점
    void process(Session* session, uint16_t protocol, const std::string& body) override;

    // ── 인증 (Customer_Auth.cpp) ─────────────────────────────
    void handleWithdraw(Session* session, const std::string& body);

    // ── 가게/메뉴 (Customer_Store.cpp) ──────────────────────
    void handleStoreList(Session* session, const std::string& body);
    void handleMenuList (Session* session, const std::string& body);

    // ── 주문 (Customer_Order.cpp) ───────────────────────────
    void handleCreateOrder  (Session* session, const std::string& body);
    void handleOrderHistory (Session* session, const std::string& body);
    void handleOrderDetail  (Session* session, const std::string& body);
    void handlePayment      (Session* session, const std::string& body);
    void handleCancelOrder  (Session* session, const std::string& body);

    // ── 리뷰 (Customer_Review.cpp) ──────────────────────────
    void handleWriteReview(Session* session, const std::string& body);
    void handleReviewList (Session* session, const std::string& body);

    // ── 마이페이지 (Customer_MyPage.cpp) ────────────────────
    void handleMyPoint        (Session* session, const std::string& body);
    void handleChangePassword (Session* session, const std::string& body);
    void handleMyInfo         (Session* session, const std::string& body);

    // ── 주소 (Customer_Address.cpp) ─────────────────────────
    void handleGetAddresses     (Session* session, const std::string& body);
    void handleSaveAddress      (Session* session, const std::string& body);
    void handleDeleteAddress    (Session* session, const std::string& body);
    void handleSetDefaultAddress(Session* session, const std::string& body);

    // ── 이미지 (Customer_Image.cpp) ─────────────────────────
    void handleGetImage(Session* session, const std::string& body);

    // ── 채팅 (Customer_Chat.cpp) ────────────────────────────
    void handleChatCreateRoom(Session* session, const std::string& body);
    void handleChatSendMsg   (Session* session, const std::string& body);
    void handleChatGetMsgs   (Session* session, const std::string& body);

    // ── Push 알림 ────────────────────────────────────────────
    void pushOrderStatus(Session* session, int orderID, int status, const std::string& msg);

private:
    CustomerHandler() : BaseHandler(ClientType::CUSTOMER, "CUSTOMER") {}
};
