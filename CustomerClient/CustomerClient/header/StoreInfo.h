#pragma once

#include <string>

#include <vector>



// [CUSTOMER] 매장 정보 데이터 모델

class StoreInfo

{

public:

    int storeID;                // 가게 고유 ID

    std::string storeName;      // 상호명 (CUS-07)

    std::string category;       // 음식 카테고리 (대분류: 족발, 피자 등) (CUS-03)

    std::string openTime;       // 영업 시간 (CUS-07)

    std::string phoneNumber;    // 전화번호 (CUS-07)

    std::string address;        // 매장 주소 (CUS-07)

    std::string holiday;        // 휴무일 (CUS-07)



    int minOrderAmount;         // 최소 주문 금액 (CUS-06)

    double distance;            // 사용자로부터의 거리 (km 단위) (CUS-06)



    // CUS-06 요구사항 구체화

    std::string deliveryPriceRange; // 배달 가격 범위 (예: "2,000원~3,500원")

    std::string deliveryTime;      // 예상 배달 시간 (예: "25~35분")

    std::string storeImageUrl;     // 매장/음식 대표 이미지 경로



    StoreInfo();

    void Clear();

};