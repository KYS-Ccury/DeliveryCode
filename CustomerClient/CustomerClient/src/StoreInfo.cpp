#include "pch.h"

#include "StoreInfo.h"



StoreInfo::StoreInfo()

{

    Clear();

}



void StoreInfo::Clear()

{

    storeID = -1;

    storeName = "";

    category = "";

    openTime = "";

    phoneNumber = "";

    address = "";

    holiday = "";

    minOrderAmount = 0;

    distance = 0.0;

    deliveryPriceRange = "";

    deliveryTime = "";

    storeImageUrl = "";
    logoUrl = "";
    description = "";

}