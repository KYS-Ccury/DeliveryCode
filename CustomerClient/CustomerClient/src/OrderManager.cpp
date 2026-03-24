#include "pch.h"

#include "OrderManager.h"



OrderManager::OrderManager()

    : m_currentStoreID(-1)

    , m_isDelivery(true)

{

    // 초기 데이터 로드 시 카테고리도 함께 설정

    m_categoryList = { "전체", "족발/보쌈", "찜/탕", "일식", "치킨", "피자", "중식", "양식" };

}



std::vector<std::string> OrderManager::GetCategoryList() {

    return m_categoryList;

}



// 테스트용 데이터 로드


void OrderManager::UpdateStoreCache(const std::vector<StoreInfo>& stores)
{
    m_allStores = stores;
}
void OrderManager::LoadStoreData() {

    m_allStores.clear();



    StoreInfo s1;

    s1.storeID = 101;

    s1.storeName = "마왕족발 대구점";

    s1.category = "족발/보쌈";

    s1.deliveryTime = "20~30분";

    s1.distance = 0.8;

    s1.minOrderAmount = 15000;

    m_allStores.push_back(s1);



    StoreInfo s2;

    s2.storeID = 102;

    s2.storeName = "황금치킨 본점";

    s2.category = "치킨";

    s2.deliveryTime = "30~40분";

    s2.distance = 1.2;

    s2.minOrderAmount = 18000;

    m_allStores.push_back(s2);

}



// 카테고리에 맞는 가게만 골라내어 로드

std::vector<StoreInfo> OrderManager::GetStoresByCategory(const std::string& category) {

    if (category == "전체") return m_allStores;



    std::vector<StoreInfo> filtered;

    for (const auto& s : m_allStores) {

        if (s.category == category) {

            filtered.push_back(s);

        }

    }

    return filtered;

}



// [고객] 장바구니 담기 (CUS-11, 18)

bool OrderManager::AddToCart(int storeID, const CartItem& item)

{

    if (m_currentStoreID != -1 && m_currentStoreID != storeID) {

        return false; // 타 매장 상품 존재 시 알림용

    }

    m_currentStoreID = storeID;

    m_cartList.push_back(item);

    return true;

}



void OrderManager::ClearCart()

{

    m_cartList.clear();

    m_currentStoreID = -1;

}

void OrderManager::LoadMenuData(int storeID, const std::string& category)
{
    m_currentStoreID = storeID;
    // 메뉴 로드 로직...
}

void OrderManager::RegisterOrderStatusCallback(std::function<void(const std::string&, int, const std::string&)> callback)
{
    // 콜백 등록 로직...
}

int OrderManager::GetTotalAmount() const

{

    int total = 0;

    for (const auto& item : m_cartList) {

        total += item.totalPrice;

    }

    return total;

}



void OrderManager::SetDeliveryType(bool isDelivery)

{

    m_isDelivery = isDelivery;

}



// [고객] 결제 및 주문 생성

bool OrderManager::ProcessOrder(const std::string& cardID, int usePoint, const std::string& couponID, const std::string& extraArg)
{



    // TODO: 서버 API 호출하여 DB에 주문 데이터 저장

    ClearCart();

    return true;

}



// [공통] 주문 취소 (CUS-17)

bool OrderManager::CancelOrder(const std::string& orderID)

{

    // TODO: 현재 주문 상태 확인 후 승인 전이면 취소 처리

    return true;

}



// [라이더/사장] 상태 변경 로직 (핵심 수정 사항)

bool OrderManager::UpdateOrderStatus(const std::string& orderID, int newStatus)

{

    // newStatus: 1(조리중), 2(배달중 - 라이더 픽업), 3(배달완료)

    // TODO: 서버 DB의 Order 테이블 상태 업데이트 및 고객에게 푸시 알림

    return true;

}



// [라이더] 배차 완료 처리

bool OrderManager::AssignRiderToOrder(const std::string& orderID, const std::string& riderID)

{

    // TODO: 주문 데이터에 라이더 ID 매칭 및 상태를 '배달준비'로 변경

    return true;

}



// [고객] 실시간 상태 조회 (CUS-15)

int OrderManager::GetLiveStatus(const std::string& orderID)

{

    // TODO: 서버에서 현재 주문의 status 컬럼 값 가져오기

    return 2; // 테스트용: 배달 중 반환

}



std::vector<OrderInfo> OrderManager::GetOrderHistory()

{

    std::vector<OrderInfo> history;

    return history;

}



int OrderManager::GetMyPoints() { return 1000; }
void OrderManager::SelectStore(int storeID)
{
    m_currentStoreID = storeID;
}

std::vector<MenuInfo> OrderManager::GetMenuData(int storeID, const std::string& category)
{
    std::vector<MenuInfo> result;

    MenuInfo m1;
    m1.menuID = storeID * 100 + 1;
    m1.menuName = "\EB\8C\80\ED\91\9C \EB\A9\94\EB\89\B4 1";
    m1.price = 12000;
    m1.subCategory = "\EC\9D\B8\EA\B8\B0\EB\A9\94\EB\89\B4";
    result.push_back(m1);

    MenuInfo m2;
    m2.menuID = storeID * 100 + 2;
    m2.menuName = "\EB\8C\80\ED\91\9C \EB\A9\94\EB\89\B4 2";
    m2.price = 15000;
    m2.subCategory = "\EC\9D\B8\EA\B8\B0\EB\A9\94\EB\89\B4";
    result.push_back(m2);

    return result;
}
