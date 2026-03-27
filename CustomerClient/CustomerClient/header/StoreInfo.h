#pragma once

#include <string>
#include <vector>

// [CUSTOMER] 매장 정보 데이터 모델
class StoreInfo
{
public:
    int storeID;                    // 가게 고유 ID
    std::string storeName;          // 상호명
    std::string category;           // 음식 카테고리
    std::string openTime;           // 영업 시간
    std::string phoneNumber;        // 전화번호
    std::string address;            // 매장 주소
    std::string holiday;            // 휴무일
    int minOrderAmount;             // 최소 주문 금액
    double distance;                // 사용자로부터의 거리 (km)
    std::string deliveryPriceRange; // 배달 가격 범위
    std::string deliveryTime;       // 예상 배달 시간
    std::string storeImageUrl;      // 매장 대표 이미지 경로 (메뉴 이미지)
    std::string logoUrl;            // ★ 음식점 로고 이미지 경로
    std::string description;        // 가게 소개글
    int         deliveryFee = 0;         // 배달비

    StoreInfo();
    void Clear();
};
