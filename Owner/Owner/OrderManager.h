#pragma once
#include <vector>
#include <afx.h> // CString 사용을 위해 필요

struct OrderInfo {
    int nID = 0;
    CString strMenu;
    CString strPrice;
    CString strStatus;
};

class OrderManager {
public:
    // 객체 생성 없이 바로 호출할 수 있도록 static으로 선언합니다.
    static std::vector<OrderInfo> FetchOrdersFromServer(int ownerId);
    static bool AcceptOrder(int orderId, int estimatedMinutes);
    static bool RejectOrder(int orderId, const CString& reason);
};