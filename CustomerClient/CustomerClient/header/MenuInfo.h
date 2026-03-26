#pragma once

#include <string>

#include <vector>



// 메뉴에 종속된 옵션 아이템 (예: 보통맛, 매운맛 / 라면사리 등)

struct OptionItem {

    int optionID;

    std::string optionName;

    int optionPrice;

};



// 옵션 그룹 (예: "맵기 선택", "추가 토핑")

struct OptionGroup {

    std::string groupName;

    bool isRequired;   // 필수 선택 여부

    std::vector<OptionItem> items;

};



// [CUSTOMER] 매장 내 개별 메뉴 정보 모델

class MenuInfo

{

public:

    int menuID;                 // 메뉴 고유 ID

    std::string menuName;       // 음식명 (CUS-09)

    std::string description;    // ★ 메뉴 설명 (서버 "desc" 키)

    int price;                  // 가격 (CUS-09)

    std::string subCategory;    // 소분류 카테고리 (인기메뉴, 메인요리 등) (CUS-08)

    std::string menuImageUrl;   // 음식 이미지 경로 (CUS-06)



    // CUS-10: 옵션 선택 관련 데이터

    std::vector<OptionGroup> optionGroups;



    MenuInfo();

    void Clear();

};