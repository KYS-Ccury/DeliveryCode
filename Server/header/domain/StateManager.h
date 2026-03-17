//  주문 상태 전이 규칙을 관리하는 객체이다.
//  서버가 흐름을 잃지 않게 상태 전이를 중앙에서 검증한다.

#pragma once

#include "Order.h"

class StateManager {
public:
    //  현재 상태에서 다음 상태로 이동 가능한지 검사한다.
    bool canTransition(OrderState from, OrderState to) const;
};