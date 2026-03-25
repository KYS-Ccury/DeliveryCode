//  ?곌껐 醫낅즺, ?≪떊 媛숈? ?ㅽ듃?뚰겕 ?곌껐 愿??怨듯넻 ?좏떥 梨낆엫??媛吏꾨떎.

#pragma once

#include <string>

class ConnectionManager {
public:
    //  ?뱀젙 ?대씪?댁뼵?몄뿉寃?臾몄옄???묐떟??蹂대궦??
    static void sendToClient(int fd, const std::string& message);
};