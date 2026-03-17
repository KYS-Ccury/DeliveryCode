//“주문 객체들을 관리하는 관리자”//  주문 매니저 구현부이다.

#include "OrderManager.h"

OrderManager::OrderManager() = default;

//  주문을 생성한다.
int OrderManager::createOrder(int userId, const std::string& menuInfo) {
    //  주문 저장소를 잠근다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  새 주문 객체를 만든다.
    Order order;
    order.orderId = m_nextOrderId++;
    order.userId = userId;
    order.menuInfo = menuInfo;
    order.state = OrderState::CREATED;

    //  저장소에 넣는다.
    m_orders[order.orderId] = order;

    //  생성된 주문 ID를 반환한다.
    return order.orderId;
}

//  주문 상태를 변경한다.
bool OrderManager::changeState(int orderId, OrderState nextState) {
    //  주문 저장소를 잠근다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  해당 주문을 찾는다.
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        //  주문이 없으면 실패한다.
        return false;
    }

    //  현재 상태에서 다음 상태로 갈 수 있는지 검사한다.
    if (!m_stateManager.canTransition(it->second.state, nextState)) {
        //  불가능한 상태 전이면 실패한다.
        return false;
    }

    //  상태를 변경한다.
    it->second.state = nextState;
    return true;
}

//  주문을 조회한다.
bool OrderManager::getOrder(int orderId, Order& outOrder) {
    //  주문 저장소를 잠근다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  주문을 찾는다.
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        //  주문이 없으면 실패한다.
        return false;
    }

    //  결과를 복사한다.
    outOrder = it->second;
    return true;
}