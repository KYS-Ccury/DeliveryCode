#pragma once
#include "MariaDBManager.h"
#include <string>
#include <vector>

// ============================================================
//  RiderDB  —  라이더 전용 DB 쿼리 모음
//
//  대상 테이블:
//    rider_profiles   (라이더 전용 정보)
//    orders           (배달 진행용 조회/변경)
//    dispatch_logs    (배차 이력)
//    order_status_logs(주문 상태 이력)
//    rider_earnings   (배달 수익)
//
//  공통 쿼리(users, payment_methods)는 CommonDB 사용
//  비밀번호 변경: CommonDB::queryCheckPassword + queryChangePassword 조합 사용
//
//  사용처:
//    Rider_Auth.cpp    → insertRiderProfile, queryRiderProfile, setOnline,
//                        changeVehicleType, changeAccountInfo
//    Rider_Delivery.cpp→ queryDispatchList, acceptDispatch, rejectDispatch,
//                        pickupDone, deliveryDone,
//                        queryTodaySummary, queryMyDispatches
//    Rider_status.cpp  → setWorkStatus, updateGps
// ============================================================
class RiderDB {
public:
    static RiderDB& getInstance() {
        static RiderDB inst;
        return inst;
    }

    // ─────────────────────────────────────────────────────────
    //  [라이더 프로필] Rider_Auth.cpp 에서 사용
    //  대상 테이블: rider_profiles
    // ─────────────────────────────────────────────────────────

    // rider_profiles INSERT (회원가입 직후 onSignup 에서 호출)
    bool insertRiderProfile(int userId, const std::string& vehicleType);

    // rider_profiles 없으면 BIKE 기본값으로 자동 생성
    bool insertRiderProfileIfMissing(int userId);

    struct RiderProfile {
        std::string name;          // users.name
        std::string phone;         // users.phone
        std::string vehicleType;   // rider_profiles.vehicle_type
        bool        isWorking   = false;
        bool        isAccepting = false;
        bool        found       = false;
    };

    // users JOIN rider_profiles → 로그인 응답용 프로필 조회
    RiderProfile queryRiderProfile(int userId);

    // is_online 컬럼 업데이트 (로그인 → TRUE, 로그아웃 → FALSE)
    bool setOnline(int userId, bool online);

    // 차량 종류 변경 (rider_profiles.vehicle_type)
    bool changeVehicleType(int userId, const std::string& vehicleType);

    // 계좌 정보 변경 (rider_profiles.bank_name 등)
    bool changeAccountInfo(int userId,
                           const std::string& bankName,
                           const std::string& accountHolder,
                           const std::string& accountNumber);

    // ─────────────────────────────────────────────────────────
    //  [배차 / 배달] Rider_Delivery.cpp 에서 사용
    //  대상 테이블: orders, restaurants, dispatch_logs,
    //              order_status_logs, rider_earnings
    // ─────────────────────────────────────────────────────────

    struct DispatchOrder {
        int         orderId     = 0;
        std::string storeName;
        std::string pickupAddr;
        std::string destAddr;
        int         deliveryFee = 0;
        int         totalPrice  = 0;
        int         elapsedSec  = 0;
    };

    // 배차 대기 목록 (orders.status='ACCEPTED', rider_id IS NULL)
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

    // 배차 수락: ACCEPTED → DELIVERING (트랜잭션)
    // dispatch_logs INSERT + order_status_logs INSERT 포함
    AcceptResult acceptDispatch(int orderId, int riderId);

    // 배차 거절: dispatch_logs INSERT
    bool rejectDispatch(int orderId, int riderId, const std::string& reason);

    // 픽업 완료: order_status_logs INSERT
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

    // 내 배달 완료 내역 목록 (최근 50건)
    std::vector<MyDispatchRecord> queryMyDispatches(int riderId);

    // ─────────────────────────────────────────────────────────
    //  [상태 / GPS] Rider_status.cpp 에서 사용
    //  대상 테이블: rider_profiles, users
    // ─────────────────────────────────────────────────────────

    // action: "ONLINE" | "OFFLINE" | "DISPATCH_ON" | "DISPATCH_OFF" | "VEHICLE"
    // vehicleType: action == "VEHICLE" 일 때만 사용
    bool setWorkStatus(int riderId,
                       const std::string& action,
                       const std::string& vehicleType = "");

    // users.latitude/longitude + rider_profiles.last_location_at 업데이트
    bool updateGps(int riderId, double lat, double lng);

private:
    RiderDB() = default;
};
