//  ?곹깭 ?꾩씠 洹쒖튃 援ы쁽遺?대떎.

#include "StateManager.h"

//  ?곹깭 ?꾩씠媛 媛?ν븳吏 ?щ?瑜?諛섑솚?쒕떎.
bool StateManager::canTransition(OrderState from, OrderState to) const {
    //  ?뺤긽 ?먮쫫 ?꾩씠 洹쒖튃???뺤쓽?쒕떎.
    if (from == OrderState::CREATED   && to == OrderState::ACCEPTED) return true;
    if (from == OrderState::ACCEPTED  && to == OrderState::COOKING)  return true;
    if (from == OrderState::COOKING   && to == OrderState::PICKUP)   return true;
    if (from == OrderState::PICKUP    && to == OrderState::DELIVERY) return true;
    if (from == OrderState::DELIVERY  && to == OrderState::DONE)     return true;

    //  ?덉쇅 ?먮쫫 ?꾩씠 洹쒖튃???뺤쓽?쒕떎.
    if (from == OrderState::CREATED   && to == OrderState::CANCELED) return true;
    if (from == OrderState::ACCEPTED  && to == OrderState::CANCELED) return true;
    if (from == OrderState::COOKING   && to == OrderState::FAILED)   return true;
    if (from == OrderState::DELIVERY  && to == OrderState::FAILED)   return true;

    //  ??洹쒖튃???놁쑝硫?遺덇??ν븳 ?꾩씠濡?蹂몃떎.
    return false;
}