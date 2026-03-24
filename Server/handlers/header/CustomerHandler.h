#pragma once
#include <string>
#include <cstdint>

class Session;

class CustomerHandler {
public:
    static void process(Session* session, uint16_t protocol,
                        const std::string& jsonBody);

    // int status 버전 (0:대기 1:조리중 2:접수 3:배달중 4:취소 등)
    static void pushOrderStatus(Session* session, int orderID,
                                int status, const std::string& msg);

private:
    // ── 공통 인증 ─────────────────────────────────────
    static void handleLogin       (Session* s, const std::string& b);
    static void handleSignup      (Session* s, const std::string& b);
    static void handleLogout      (Session* s, const std::string& b);
    static void handleGetProfile  (Session* s, const std::string& b);
    static void handleWithdraw    (Session* s, const std::string& b);

    // ── 가게/메뉴 ────────────────────────────────────
    static void handleStoreList   (Session* s, const std::string& b);
    static void handleMenuList    (Session* s, const std::string& b);

    // ── 주문 ─────────────────────────────────────────
    static void handleCreateOrder (Session* s, const std::string& b);
    static void handleOrderHistory(Session* s, const std::string& b);
    static void handleOrderDetail (Session* s, const std::string& b);
    static void handleCancelOrder (Session* s, const std::string& b);
    static void handlePayment     (Session* s, const std::string& b);

    // ── 리뷰 ─────────────────────────────────────────
    static void handleWriteReview (Session* s, const std::string& b);
    static void handleReviewList  (Session* s, const std::string& b);

    // ── 마이페이지 ───────────────────────────────────
    static void handleRegister    (Session* s, const std::string& b);
    static void handleGetMyInfo   (Session* s, const std::string& b);
    static void handleUpdateMyInfo(Session* s, const std::string& b);
    static void handleAddCard     (Session* s, const std::string& b);
    static void handleGetCards    (Session* s, const std::string& b);
    static void handleDeleteCard  (Session* s, const std::string& b);
    static void handleGetCoupons  (Session* s, const std::string& b);
    static void handleGetPoints   (Session* s, const std::string& b);
};
