#pragma once
#include <string>
#include <vector>
#include "MenuInfo.h" // OptionItem 구조체 참조를 위해 포함

// 장바구니에 담긴 하나의 항목 (메뉴 + 선택 옵션들)
class CartItem
{
public:
    int menuID;                     // 메뉴 고유 ID
    std::string menuName;           // 음식명
    int basePrice;                  // 메뉴 기본 가격
    int quantity;                   // 주문 수량

    // 사용자가 선택한 옵션들의 리스트
    std::vector<OptionItem> selectedOptions;

    int totalPrice;                 // (기본가 + 옵션가 합산) * 수량
    int storeID;                    // 장바구니 정합성 체크용 (어느 가게 메뉴인지)

    CartItem();
    void CalculateTotalPrice();     // 총 가격 계산 함수
};