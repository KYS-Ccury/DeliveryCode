//  상태 전이 규칙 구현부이다.

#include "StateManager.h"

//  상태 전이가 가능한지 여부를 반환한다.
bool StateManager::canTransition(OrderState from, OrderState to) const {
    //  정상 흐름 전이 규칙을 정의한다.
    if (from == OrderState::CREATED   && to == OrderState::ACCEPTED) return true;
    if (from == OrderState::ACCEPTED  && to == OrderState::COOKING)  return true;
    if (from == OrderState::COOKING   && to == OrderState::PICKUP)   return true;
    if (from == OrderState::PICKUP    && to == OrderState::DELIVERY) return true;
    if (from == OrderState::DELIVERY  && to == OrderState::DONE)     return true;

    //  예외 흐름 전이 규칙을 정의한다.
    if (from == OrderState::CREATED   && to == OrderState::CANCELED) return true;
    if (from == OrderState::ACCEPTED  && to == OrderState::CANCELED) return true;
    if (from == OrderState::COOKING   && to == OrderState::FAILED)   return true;
    if (from == OrderState::DELIVERY  && to == OrderState::FAILED)   return true;

    //  위 규칙에 없으면 불가능한 전이로 본다.
    return false;
}