#include "pch.h"
#include "OrderInfo.h"

OrderInfo::OrderInfo()
{
    Clear();
}

void OrderInfo::Clear()
{
    orderID = "";
    storeID = -1;
    storeName = "";
    orderDateTime = "";
    totalPayment = 0;
    deliveryStatus = STATUS_WAITING;
    isDelivery = true;
    deliveryAddress = "";
    riderID = "";
}