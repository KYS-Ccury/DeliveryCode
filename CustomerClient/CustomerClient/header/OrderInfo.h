#pragma once
#include <string>
#include <vector>

// 배달 상태 정의 (가독성을 위한 enum)
enum DeliveryStatus {
    STATUS_WAITING = 0,   // 접수 대기
    STATUS_PREPARING = 1, // 조리 중
    STATUS_DELIVERING = 2,// 배달 중 (라이더 픽업)
    STATUS_COMPLETE = 3,  // 배달 완료
    STATUS_CANCELED = 4   // 주문 취소
};

class OrderInfo
{
public:
    std::string orderID;      // 주문 번호 (예: 20240522-0001)
    int storeID;              // 가게 고유 ID
    std::string storeName;    // 가게 이름 (내역 조회 시 편의용)
    std::string orderDateTime;// 주문 일시

    int totalPayment;         // 최종 결제 금액
    int deliveryStatus;       // 현재 배달 상태 (DeliveryStatus 사용)
    bool isDelivery;          // 수령 방법 (true: 배달, false: 포장)

    // 상세 내역 조회를 위한 추가 필드
    std::string deliveryAddress; // 배달지 주소
    std::string riderID;         // 배정된 라이더 ID

    OrderInfo();
    void Clear();
};