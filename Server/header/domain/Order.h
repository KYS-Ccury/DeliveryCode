//  二쇰Ц ?꾨찓??媛앹껜?대떎.
//  二쇰Ц? ?⑥닚 ?곗씠?곌? ?꾨땲???곹깭瑜?媛吏??媛앹껜?쇰뒗 愿?먯쑝濡??뺤쓽?쒕떎.

#pragma once

#include <string>

//  二쇰Ц ?곹깭 ?닿굅?뺤씠??
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

//  二쇰Ц 媛앹껜?대떎.
struct Order {
    //  二쇰Ц ID?대떎.
    int orderId = 0;

    //  二쇰Ц???ъ슜??ID?대떎.
    int userId = 0;

    //  硫붾돱 ?뺣낫 臾몄옄?댁씠??
    std::string menuInfo;

    //  ?꾩옱 二쇰Ц ?곹깭?대떎.
    OrderState state = OrderState::CREATED;
};