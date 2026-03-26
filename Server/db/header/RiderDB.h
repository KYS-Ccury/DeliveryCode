#pragma once
#include "MariaDBManager.h"
#include <string>
#include <vector>

// ============================================================
//  RiderDB  —  라이더 전용 DB 쿼리 모음
//  Rider_Auth / Rider_Delivery / Rider_status 에서
//  직접 MariaDB 호출하던 쿼리를 모두 여기에 집결
// ============================================================
class RiderDB {
public:
    static RiderDB& getInstance() {
        static RiderDB inst;
        return inst;
    }

    // ─────────────────────────────────────────────────────────
    //  [인증 관련] Rider_Auth.cpp 에서 사용
    // ─────────────────────────────────────────────────────────

    // 라이더 프로필 생성 (회원가입 직후)
    bool insertRiderProfile(int userId, const std::string& vehicleType);

    // 라이더 프로필 자동 생성 (없으면 BIKE 기본값)
    bool insertRiderProfileIfMissing(int userId);

    struct RiderProfile {
        std::string name;
        std::string phone;
        std::string vehicleType;
        bool        isWorking   = false;
        bool        isAccepting = false;
        bool        found       = false;
    };

    // 로그인 시 라이더 정보 조회 (users + rider_profiles JOIN)
    RiderProfile queryRiderProfile(int userId);

    // 온라인 상태 ON (로그인)
    bool setOnline(int userId, bool online);

    // 비밀번호 변경 (CommonDB 위임 가능하지만 라이더 전용으로 여기 배치)
    bool changePassword(int userId,
                        const std::string& curPw,
                        const std::string& newPw);

    // 차량 종류 변경
    bool changeVehicleType(int userId, const std::string& vehicleType);

    // 계좌 정보 변경 (스키마에 컬럼 있을 경우)
    bool changeAccountInfo(int userId,
                           const std::string& bankName,
                           const std::string& accountHolder,
                           const std::string& accountNumber);

    // ─────────────────────────────────────────────────────────
    //  [배달 관련] Rider_Delivery.cpp 에서 사용
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

    // 배차 대기 목록 조회 (status=ACCEPTED, rider_id IS NULL)
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

    // 배차 수락: ACCEPTED → DELIVERING (트랜잭션)
    // 반환: {성공 여부, 주문 상세}
    struct AcceptResult {
        bool        ok = false;
        OrderDetail detail;
    };
    AcceptResult acceptDispatch(int orderId, int riderId);

    // 배차 거절 로그 기록
    bool rejectDispatch(int orderId, int riderId, const std::string& reason);

    // 픽업 완료 로그 기록
    bool pickupDone(int orderId, int riderId);

    struct DeliveryDoneResult {
        bool ok          = false;
        int  deliveryFee = 0;
        int  customerId  = 0;
    };

    // 배달 완료: DELIVERING → DONE + 수익 기록 (트랜잭션)
    DeliveryDoneResult deliveryDone(int orderId, int riderId);

    struct MyDispatchSummary {
        int todayCount = 0;
        int todayFee   = 0;
    };

    // 오늘의 배달 요약 (건수 + 수익)
    MyDispatchSummary queryTodaySummary(int riderId);

    struct MyDispatchRecord {
        int         orderId     = 0;
        std::string orderCode;
        std::string storeName;
        int         deliveryFee = 0;
        std::string status;
        std::string createdAt;
    };

    // 내 배달 내역 목록 (최근 50건)
    std::vector<MyDispatchRecord> queryMyDispatches(int riderId);

    // ─────────────────────────────────────────────────────────
    //  [상태/GPS 관련] Rider_status.cpp 에서 사용
    // ─────────────────────────────────────────────────────────

    // 출퇴근 / 배차수락 ON·OFF 상태 변경
    bool setWorkStatus(int riderId, const std::string& action,
                       const std::string& vehicleType = "");

    // GPS 위치 업데이트
    bool updateGps(int riderId, double lat, double lng);

private:
    RiderDB() = default;
    std::string escape(const std::string& s);
    std::string makeOrderCode(int orderId);
};
