//?쒖＜臾?媛앹껜?ㅼ쓣 愿由ы븯??愿由ъ옄??/  二쇰Ц 留ㅻ땲? 援ы쁽遺?대떎.

#include "OrderManager.h"

OrderManager::OrderManager() = default;

//  二쇰Ц???앹꽦?쒕떎.
int OrderManager::createOrder(int userId, const std::string& menuInfo) {
    //  二쇰Ц ??μ냼瑜??좉렐??
    std::lock_guard<std::mutex> lock(m_mtx);

    //  ??二쇰Ц 媛앹껜瑜?留뚮뱺??
    Order order;
    order.orderId = m_nextOrderId++;
    order.userId = userId;
    order.menuInfo = menuInfo;
    order.state = OrderState::CREATED;

    //  ??μ냼???ｋ뒗??
    m_orders[order.orderId] = order;

    //  ?앹꽦??二쇰Ц ID瑜?諛섑솚?쒕떎.
    return order.orderId;
}

//  二쇰Ц ?곹깭瑜?蹂寃쏀븳??
bool OrderManager::changeState(int orderId, OrderState nextState) {
    //  二쇰Ц ??μ냼瑜??좉렐??
    std::lock_guard<std::mutex> lock(m_mtx);

    //  ?대떦 二쇰Ц??李얜뒗??
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        //  二쇰Ц???놁쑝硫??ㅽ뙣?쒕떎.
        return false;
    }

    //  ?꾩옱 ?곹깭?먯꽌 ?ㅼ쓬 ?곹깭濡?媛????덈뒗吏 寃?ы븳??
    if (!m_stateManager.canTransition(it->second.state, nextState)) {
        //  遺덇??ν븳 ?곹깭 ?꾩씠硫??ㅽ뙣?쒕떎.
        return false;
    }

    //  ?곹깭瑜?蹂寃쏀븳??
    it->second.state = nextState;
    return true;
}

//  二쇰Ц??議고쉶?쒕떎.
bool OrderManager::getOrder(int orderId, Order& outOrder) {
    //  二쇰Ц ??μ냼瑜??좉렐??
    std::lock_guard<std::mutex> lock(m_mtx);

    //  二쇰Ц??李얜뒗??
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        //  二쇰Ц???놁쑝硫??ㅽ뙣?쒕떎.
        return false;
    }

    //  寃곌낵瑜?蹂듭궗?쒕떎.
    outOrder = it->second;
    return true;
}