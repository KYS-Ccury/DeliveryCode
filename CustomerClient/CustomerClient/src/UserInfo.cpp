#include "pch.h"
#include "UserInfo.h"

UserInfo::UserInfo()
{
    Clear();
}

void UserInfo::Clear()
{
    id = "";
    pw = "";
    address = "";
    role = ROLE_CUSTOMER; // 기본값은 고객
    points = 0;

    // 리스트 및 벡터 초기화
    cardList.clear();
    couponList.clear();

    // 라이더/사장 관련 추가 필드 초기화
    vehicleType = "";
    isWorking = false;
    deliveryCount = 0;
    ownedStoreID = -1;
}