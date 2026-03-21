#pragma once
#include <string>
#include <vector>

// ROLE 정의 (열거형으로 관리하면 코드 가독성이 좋음)
enum UserRole {
    ROLE_CUSTOMER = 0,
    ROLE_OWNER = 1,
    ROLE_RIDER = 2,
    ROLE_ADMIN = 3
};

struct CardInfo {
    std::string cardName;
    std::string cardNumber;
};

class UserInfo
{
public:
    // --- [공통 정보] ---
    std::string id;
    std::string pw;
    std::string address;
    int role;                // UserRole 값 (0:고객, 1:사장, 2:라이더 등)

    // --- [고객 전용] ---
    int points;
    std::vector<CardInfo> cardList;
    std::vector<std::string> couponList;

    // --- [라이더 전용] (추가됨) ---
    std::string vehicleType; // 오토바이, 자전거, 도보 등
    bool isWorking;          // 현재 업무 중(ON/OFF) 여부
    int deliveryCount;       // 오늘 완료한 배달 건수

    // --- [사장 전용] (추가됨) ---
    int ownedStoreID;        // 본인이 운영하는 매장 ID

    UserInfo();
    void Clear();
};