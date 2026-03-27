#pragma once
#include <afxwin.h>

struct RiderSession {
    int     riderId       = 0;
    CString loginId;
    CString name;
    CString phone;
    CString deliveryRegion;
    CString vehicleType;
    CString bankName;
    CString accountHolder;
    CString accountNumber;
    bool      isLoggedIn      = false;
    bool      isOnline        = false;
    ULONGLONG drivingStartTick = 0;  // GetTickCount64() when driving started
    ULONGLONG totalDriveSec   = 0;  // accumulated seconds from previous sessions
    void Clear() {
        riderId = 0;
        loginId = name = phone = _T("");
        deliveryRegion = vehicleType = _T("");
        bankName = accountHolder = accountNumber = _T("");
        isLoggedIn = isOnline = false;
        drivingStartTick = totalDriveSec = 0;
    }
};

struct CurrentOrder {
    int     orderId       = 0;
    CString orderCode;
    CString pickupAddress;
    CString deliveryAddress;
    CString customerRequest;
    int     deliveryFee   = 0;
    CString status;
    bool IsActive() const { return orderId > 0; }
    void Clear() {
        orderId = deliveryFee = 0;
        orderCode = pickupAddress = deliveryAddress = _T("");
        customerRequest = status = _T("");
    }
};
