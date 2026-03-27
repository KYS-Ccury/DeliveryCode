#pragma once
#include <afx.h>

class StoreManager {
public:
    // 영업 상태(is_open)를 서버에 업데이트하는 함수
    static bool UpdateStoreStatus(int ownerId, bool bOpen);
};