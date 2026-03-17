//  주문 객체들을 관리하는 매니저이다.
//  생성, 조회, 상태 변경 같은 주문 관련 책임을 가진다.

#pragma once

#include <unordered_map>
#include <mutex>
#include "Order.h"
#include "StateManager.h"

class OrderManager {
public:
    //  생성자이다.
    OrderManager();

    //  주문을 생성하고 주문 ID를 반환한다.
    int createOrder(int userId, const std::string& menuInfo);

    //  주문 상태를 변경한다.
    bool changeState(int orderId, OrderState nextState);

    //  주문을 조회한다.
    bool getOrder(int orderId, Order& outOrder);

private:
    //  주문 저장소이다.
    std::unordered_map<int, Order> m_orders;

    //  주문 저장소 보호용 뮤텍스이다.
    std::mutex m_mtx;

    //  주문 ID 자동 증가 값이다.
    int m_nextOrderId = 1;

    //  상태 검증 객체이다.
    StateManager m_stateManager;
};