#pragma once
// ============================================================
//  CustomerHandler.h
//
//  [수정] 주소 관련 핸들러 4개 추가
//    213  handleGetAddresses      — 주소 목록 조회
//    214  handleSaveAddress       — 주소 추가
//    215  handleDeleteAddress     — 주소 삭제
//    216  handleSetDefaultAddress — 기본 주소 설정
// ============================================================
#include "Basehandler.h"

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
    void onSignup      (Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout      (Session* session, int userId) override;
    void onGetProfile  (Session* session, int userId, const nlohmann::json& reqBody) override;

private:
    // 100번대 기타
    void handleWithdraw    (Session* s, const std::string& b); // 105

    // 고객 전용 (CmdCustomer 200~212)
    void handleStoreList   (Session* s, const std::string& b); // 200
    void handleMenuList    (Session* s, const std::string& b); // 201
    void handleCreateOrder (Session* s, const std::string& b); // 202
    void handleOrderHistory(Session* s, const std::string& b); // 203
    void handleOrderDetail (Session* s, const std::string& b); // 204
    void handlePayment     (Session* s, const std::string& b); // 205
    void handleWriteReview (Session* s, const std::string& b); // 206
    void handleReviewList  (Session* s, const std::string& b); // 207
    void handleCancelOrder (Session* s, const std::string& b); // 208
    void handleMyPoint      (Session* s, const std::string& b); // 209
    void handleChangePassword(Session* s, const std::string& b); // 211
    void handleMyInfo       (Session* s, const std::string& b); // 212

    // ★ 주소 관련 (CmdCustomer 213~216) — 신규
    void handleGetAddresses     (Session* s, const std::string& b); // 213
    void handleSaveAddress      (Session* s, const std::string& b); // 214
    void handleDeleteAddress    (Session* s, const std::string& b); // 215
    void handleSetDefaultAddress(Session* s, const std::string& b); // 216

    // ── 채팅 (CmdChat 600~602) ──────────────────────────────
    void handleChatCreateRoom(Session* s, const std::string& b); // 600
    void handleChatSendMsg   (Session* s, const std::string& b); // 601
    void handleChatGetMsgs   (Session* s, const std::string& b); // 602
};
