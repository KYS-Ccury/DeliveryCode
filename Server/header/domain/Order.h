//  주문 도메인 객체이다.
//  주문은 단순 데이터가 아니라 상태를 가지는 객체라는 관점으로 정의한다.

#pragma once

#include <string>

//  주문 상태 열거형이다.
enum class OrderState {
    CREATED,
    ACCEPTED,
    COOKING,
    PICKUP,
    DELIVERY,
    DONE,
    CANCELED,
    FAILED
};

//  주문 객체이다.
struct Order {
    //  주문 ID이다.
    int orderId = 0;

    //  주문한 사용자 ID이다.
    int userId = 0;

    //  메뉴 정보 문자열이다.
    std::string menuInfo;

    //  현재 주문 상태이다.
    OrderState state = OrderState::CREATED;
};