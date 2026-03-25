//  二쇰Ц 媛앹껜?ㅼ쓣 愿由ы븯??留ㅻ땲??대떎.
//  ?앹꽦, 議고쉶, ?곹깭 蹂寃?媛숈? 二쇰Ц 愿??梨낆엫??媛吏꾨떎.

#pragma once

#include <unordered_map>
#include <mutex>
#include "Order.h"
#include "StateManager.h"

class OrderManager {
public:
    //  ?앹꽦?먯씠??
    OrderManager();

    //  二쇰Ц???앹꽦?섍퀬 二쇰Ц ID瑜?諛섑솚?쒕떎.
    int createOrder(int userId, const std::string& menuInfo);

    //  二쇰Ц ?곹깭瑜?蹂寃쏀븳??
    bool changeState(int orderId, OrderState nextState);

    //  二쇰Ц??議고쉶?쒕떎.
    bool getOrder(int orderId, Order& outOrder);

private:
    //  二쇰Ц ??μ냼?대떎.
    std::unordered_map<int, Order> m_orders;

    //  二쇰Ц ??μ냼 蹂댄샇??裕ㅽ뀓?ㅼ씠??
    std::mutex m_mtx;

    //  二쇰Ц ID ?먮룞 利앷? 媛믪씠??
    int m_nextOrderId = 1;

    //  ?곹깭 寃利?媛앹껜?대떎.
    StateManager m_stateManager;
};