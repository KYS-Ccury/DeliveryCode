// Server/header/server/Task.h

#pragma once
#include "Session.h"
#include "../../Common/Packet.h"

// ?먯뿉 ?ㅼ뼱媛???묒뾽 ?⑥쐞
struct Task {
    Session* session; // ?붿껌 蹂대궦 ?대씪?댁뼵??
    Packet packet;    // ?붿껌 ?곗씠??
};