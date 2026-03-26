#pragma once
#include "MariaDBManager.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class RiderDB {
public:

    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);
    static void createProfile(int userId); // 리턴 타입이 json이면 nlohmann::json으로 맞춰주세요.
    static nlohmann::json loginHook(int userId);

    static RiderDB& getInstance() {
        static RiderDB inst;
        return inst;
    }


    bool insertRiderProfile(int userId, const std::string& vehicleType);

    bool insertRiderProfileIfMissing(int userId);

    struct RiderProfile {
        std::string name;          // users.name
        std::string phone;         // users.phone
        std::string vehicleType;   // rider_profiles.vehicle_type
        bool        isWorking   = false;
        bool        isAccepting = false;
        bool        found       = false;
    };

    RiderProfile queryRiderProfile(int userId);


    bool setOnline(int userId, bool online);

    bool changeVehicleType(int userId, const std::string& vehicleType);


    bool changeAccountInfo(int userId,
                           const std::string& bankName,
                           const std::string& accountHolder,
                           const std::string& accountNumber);

    struct DispatchOrder {
        int         orderId     = 0;
        std::string storeName;
        std::string pickupAddr;
        std::string destAddr;
        int         deliveryFee = 0;
        int         totalPrice  = 0;
        int         elapsedSec  = 0;
    };


    std::vector<DispatchOrder> queryDispatchList();

    struct OrderDetail {
        int         orderId     = 0;
        std::string storeName;
        std::string pickupAddr;
        std::string storePhone;
        std::string destAddr;
        int         deliveryFee = 0;
        int         totalPrice  = 0;
        int         customerId  = 0;
        bool        found       = false;
    };

    struct AcceptResult {
        bool        ok = false;
        OrderDetail detail;
    };


    AcceptResult acceptDispatch(int orderId, int riderId);


    bool rejectDispatch(int orderId, int riderId, const std::string& reason);


    bool pickupDone(int orderId, int riderId);

    struct DeliveryDoneResult {
        bool ok          = false;
        int  deliveryFee = 0;
        int  customerId  = 0;
    };

    // 배달 완료: DELIVERING → DONE + rider_earnings INSERT (트랜잭션)
    DeliveryDoneResult deliveryDone(int orderId, int riderId);

    struct MyDispatchSummary {
        int todayCount = 0;
        int todayFee   = 0;
    };

    // 오늘 배달 요약 (건수 + 수익합계)
    MyDispatchSummary queryTodaySummary(int riderId);

    struct MyDispatchRecord {
        int         orderId     = 0;
        std::string orderCode;
        std::string storeName;
        int         deliveryFee = 0;
        std::string status;
        std::string createdAt;
    };


    std::vector<MyDispatchRecord> queryMyDispatches(int riderId);

    bool setWorkStatus(int riderId,
                       const std::string& action,
                       const std::string& vehicleType = "");


    bool updateGps(int riderId, double lat, double lng);

private:
    RiderDB() = default;
};
