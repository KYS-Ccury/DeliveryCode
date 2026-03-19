// Server/header/server/Task.h

#pragma once
#include "Session.h"
#include "../../Common/Packet.h"

// 큐에 들어가는 작업 단위
struct Task {
    Session* session; // 요청 보낸 클라이언트
    Packet packet;    // 요청 데이터
};