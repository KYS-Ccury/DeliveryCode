#include "pch.h"
#include "CartItem.h"

CartItem::CartItem()
    : menuID(-1)
    , menuName("")
    , basePrice(0)
    , quantity(1)
    , totalPrice(0)
    , storeID(-1)
{
}

// 선택된 옵션들과 수량을 바탕으로 최종 가격을 계산함
void CartItem::CalculateTotalPrice()
{
    int optionSum = 0;
    for (const auto& opt : selectedOptions) {
        optionSum += opt.optionPrice;
    }

    totalPrice = (basePrice + optionSum) * quantity;
}

