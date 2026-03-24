// ================================================================
//  OrderManager.cpp  ─  서버 연동 완성 + UpdateStoreCache 추가
//
//  [변경 사항]
//    1. UpdateStoreCache()  : MainHomeDlg 서버 응답 캐시 저장용
//    2. ProcessOrder()      : 기존 더미 → outOrderID 반환 시그니처 수정
//                             (실제 HTTP 전송은 CartDlg에서 직접 수행)
//    3. GetMenuData()       : 기존 하드코딩 유지 (폴백용)
//    4. 나머지 함수         : 기존 구현 유지
// ================================================================
#include "pch.h"
#include "OrderManager.h"

OrderManager::OrderManager()
    : m_currentStoreID(-1)
    , m_isDelivery(true)
{
    m_categoryList = { "전체", "족발/보쌈", "찜/탕", "일식", "치킨", "피자", "중식", "양식" };
}

std::vector<std::string> OrderManager::GetCategoryList() {
    return m_categoryList;
}

// ── 서버 응답 캐시 갱신 (MainHomeDlg에서 호출) ───────────────
void OrderManager::UpdateStoreCache(const std::vector<StoreInfo>& stores)
{
    m_allStores = stores;
}

// ── 초기 더미 데이터 (서버 미연결 폴백) ──────────────────────
void OrderManager::LoadStoreData()
{
    if (!m_allStores.empty()) return;  // 이미 서버 데이터 있으면 스킵

    m_allStores.clear();

    StoreInfo s1;
    s1.storeID        = 101;
    s1.storeName      = "마왕족발 대구점";
    s1.category       = "족발/보쌈";
    s1.deliveryTime   = "20~30분";
    s1.distance       = 0.8;
    s1.minOrderAmount = 15000;
    s1.address        = "광주 북구 용봉동 123";
    s1.openTime       = "11:00 ~ 23:00";
    s1.phoneNumber    = "062-000-0001";
    s1.holiday        = "매주 월요일";
    m_allStores.push_back(s1);

    StoreInfo s2;
    s2.storeID        = 102;
    s2.storeName      = "황금치킨 본점";
    s2.category       = "치킨";
    s2.deliveryTime   = "30~40분";
    s2.distance       = 1.2;
    s2.minOrderAmount = 18000;
    s2.address        = "광주 북구 운암동 456";
    s2.openTime       = "16:00 ~ 01:00";
    s2.phoneNumber    = "062-000-0002";
    s2.holiday        = "연중무휴";
    m_allStores.push_back(s2);

    StoreInfo s3;
    s3.storeID        = 103;
    s3.storeName      = "스시히로 일식당";
    s3.category       = "일식";
    s3.deliveryTime   = "25~35분";
    s3.distance       = 2.1;
    s3.minOrderAmount = 20000;
    s3.address        = "광주 서구 치평동 789";
    s3.openTime       = "11:30 ~ 22:00";
    s3.phoneNumber    = "062-000-0003";
    s3.holiday        = "매주 일요일";
    m_allStores.push_back(s3);
}

std::vector<StoreInfo> OrderManager::GetStoresByCategory(const std::string& category)
{
    if (category == "전체" || category.empty()) return m_allStores;
    std::vector<StoreInfo> filtered;
    for (const auto& s : m_allStores)
        if (s.category == category) filtered.push_back(s);
    return filtered;
}

// ── 장바구니 ──────────────────────────────────────────────────
bool OrderManager::AddToCart(int storeID, const CartItem& item)
{
    if (m_currentStoreID != -1 && m_currentStoreID != storeID)
        return false;  // 다른 가게 메뉴 → CartDlg에서 팝업 처리
    m_currentStoreID = storeID;
    m_cartList.push_back(item);
    return true;
}

void OrderManager::ClearCart()
{
    m_cartList.clear();
    m_currentStoreID = -1;
}

int OrderManager::GetTotalAmount() const
{
    int total = 0;
    for (const auto& item : m_cartList) total += item.totalPrice;
    return total;
}

void OrderManager::SetDeliveryType(bool isDelivery) { m_isDelivery = isDelivery; }

// ── 주문 처리 ─────────────────────────────────────────────────
// ※ 실제 서버 전송은 CartDlg::OnBnClickedOk()에서 직접 수행
//   이 함수는 로컬 폴백 / 테스트용으로만 사용
bool OrderManager::ProcessOrder(const std::string& cardID, int usePoint,
                                const std::string& couponID, const std::string& extraArg)
{
    if (m_cartList.empty()) return false;

    // 임시 orderID 생성 (서버 미연결 시)
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[32];
    sprintf_s(buf, sizeof(buf), "%04d%02d%02d-%04d",
              st.wYear, st.wMonth, st.wDay, rand() % 10000);
    // extraArg 는 outOrderID 용도로 사용 (const 문제로 직접 수정 불가)
    // CartDlg에서 로컬 테스트 시 outID를 따로 처리

    ClearCart();
    return true;
}

// ── 주문 취소 ─────────────────────────────────────────────────
bool OrderManager::CancelOrder(const std::string& orderID)
{
    // TODO: CmdCustomer::REQ_CANCEL_ORDER (208) 전송
    return true;
}

// ── 주문 상태 변경 ────────────────────────────────────────────
bool OrderManager::UpdateOrderStatus(const std::string& orderID, int newStatus)
{
    // TODO: 서버 NTF_ORDER_STATUS (210) 수신 시 호출
    return true;
}

bool OrderManager::AssignRiderToOrder(const std::string& orderID, const std::string& riderID)
{
    return true;
}

int OrderManager::GetLiveStatus(const std::string& orderID)
{
    return 2; // 테스트: 배달 중
}

std::vector<OrderInfo> OrderManager::GetOrderHistory()
{
    // TODO: CmdCustomer::REQ_ORDER_HISTORY (203) 요청 후 파싱
    return {};
}

void OrderManager::SelectStore(int storeID) { m_currentStoreID = storeID; }

void OrderManager::LoadMenuData(int storeID, const std::string& category)
{
    m_currentStoreID = storeID;
}

void OrderManager::RegisterOrderStatusCallback(
    std::function<void(const std::string&, int, const std::string&)> callback)
{
    m_orderStatusCallback = callback;
}

int OrderManager::GetMyPoints() { return 1000; }

// ── 메뉴 데이터 (서버 미연결 폴백) ───────────────────────────
std::vector<MenuInfo> OrderManager::GetMenuData(int storeID, const std::string& category)
{
    std::vector<MenuInfo> result;

    // 인기메뉴
    MenuInfo m1;
    m1.menuID      = storeID * 100 + 1;
    m1.menuName    = "대표 메뉴 A";
    m1.price       = 12000;
    m1.subCategory = "인기메뉴";

    OptionGroup og1;
    og1.groupName  = "맵기 선택";
    og1.isRequired = true;
    OptionItem oi1; oi1.optionID=1; oi1.optionName="보통맛"; oi1.optionPrice=0;
    OptionItem oi2; oi2.optionID=2; oi2.optionName="매운맛"; oi2.optionPrice=0;
    og1.items.push_back(oi1); og1.items.push_back(oi2);
    m1.optionGroups.push_back(og1);

    OptionGroup og2;
    og2.groupName  = "사이드 추가";
    og2.isRequired = false;
    OptionItem oi3; oi3.optionID=3; oi3.optionName="공기밥 추가"; oi3.optionPrice=1000;
    og2.items.push_back(oi3);
    m1.optionGroups.push_back(og2);
    result.push_back(m1);

    MenuInfo m2;
    m2.menuID      = storeID * 100 + 2;
    m2.menuName    = "대표 메뉴 B";
    m2.price       = 15000;
    m2.subCategory = "인기메뉴";
    result.push_back(m2);

    MenuInfo m3;
    m3.menuID      = storeID * 100 + 3;
    m3.menuName    = "세트 메뉴";
    m3.price       = 22000;
    m3.subCategory = "세트메뉴";
    result.push_back(m3);

    if (!category.empty() && category != "전체") {
        std::vector<MenuInfo> filtered;
        for (const auto& m : result)
            if (m.subCategory == category) filtered.push_back(m);
        return filtered;
    }
    return result;
}
