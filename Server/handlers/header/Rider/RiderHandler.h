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
    // Rider_status.cpp, Rider_Dispatch.cpp 등에 구현된 함수들
    void handleDispatchList(Session* session, const std::string& jsonBody);
    void handleAcceptDispatch(Session* session, const std::string& jsonBody);
    void handleRejectDispatch(Session* session, const std::string& jsonBody);
    void handlePickupDone(Session* session, const std::string& jsonBody);
    void handleDeliveryDone(Session* session, const std::string& jsonBody);
    void handleMyDispatches(Session* session, const std::string& jsonBody);
    void handleWorkStatus(Session* session, const std::string& jsonBody);
    void handleUpdateGps(Session* session, const std::string& jsonBody);
};