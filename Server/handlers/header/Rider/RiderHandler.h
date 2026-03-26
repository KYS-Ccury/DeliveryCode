#pragma once
#include "Basehandler.h"

class RiderHandler : public BaseHandler {
private:
    RiderHandler() : BaseHandler(ClientType::RIDER, "RIDER") {}

public:
    static RiderHandler& getInstance() {
        static RiderHandler instance;
        return instance;
    }

    void process(Session* session, uint16_t protocol, const std::string& jsonBody) override;
    
    // 외부 호출용 (Admin에서 사용)
    bool pushDispatch(int riderFd, int orderId, const std::string& storeName, 
                      const std::string& pickupAddr, const std::string& destAddr, int deliveryFee);

protected:
    void onSignup(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLoginSuccess(Session* session, int userId, const nlohmann::json& reqBody) override;
    void onLogout(Session* session, int userId) override;
    void onGetProfile(Session* session, int userId, const nlohmann::json& reqBody) override;
private:
    // Rider_status.cpp, Rider_Delivery.cpp 에 구현된 배달/상태 핸들러
    void handleDispatchList(Session* session, const std::string& jsonBody);
    void handleAcceptDispatch(Session* session, const std::string& jsonBody);
    void handleRejectDispatch(Session* session, const std::string& jsonBody);
    void handlePickupDone(Session* session, const std::string& jsonBody);
    void handleDeliveryDone(Session* session, const std::string& jsonBody);
    void handleMyDispatches(Session* session, const std::string& jsonBody);
    void handleWorkStatus(Session* session, const std::string& jsonBody);
    void handleUpdateGps(Session* session, const std::string& jsonBody);

    // RiderChatHandler.cpp 에 구현된 채팅 핸들러 (600번대)
    // DB 쿼리는 ChatDB 클래스에 완전히 위임한다.
    void handleChatCreateRoom(Session* session, const std::string& jsonBody);
    void handleChatSendMsg   (Session* session, const std::string& jsonBody);
    void handleChatGetMsgs   (Session* session, const std::string& jsonBody);
};