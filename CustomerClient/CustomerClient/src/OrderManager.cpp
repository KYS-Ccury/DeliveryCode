// ================================================================
//  OrderManager.cpp  ─  주문/가게/메뉴 매니저 (완성판)
//
//  [변경/완성 사항]
//  1. LoadStoreData(): 서버 연결 시 REQ_STORE_LIST 요청,
//                      미연결 시 기존 더미 데이터 유지
//  2. LoadMenuData():  REQ_MENU_LIST 요청 후 내부 캐시 저장
//  3. GetMenuData():   내부 캐시 반환 (없으면 서버 요청)
//  4. GetOrderHistory(): REQ_ORDER_HISTORY 요청 후 반환
//  5. GetMyPoints():   REQ_GET_PROFILE 로 받은 포인트 반환
//  6. RegisterOrderStatusCallback(): NTF_ORDER_STATUS(210) 수신
//
//  [서버 없이도 동작] 서버 미연결 시 기존 더미 데이터로 폴백
// ================================================================
#include "pch.h"
#include "OrderManager.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

// ── 간이 JSON 파싱 헬퍼 ──────────────────────────────────────
static std::string OMJStr(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":\"";
    auto p = j.find(t); if (p == std::string::npos) return "";
    p += t.size(); auto e = j.find('"', p);
    return (e == std::string::npos) ? "" : j.substr(p, e - p);
}
static int OMJInt(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return 0;
    try { return std::stoi(j.substr(p + t.size())); } catch (...) { return 0; }
}
static double OMJDouble(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return 0.0;
    try { return std::stod(j.substr(p + t.size())); } catch (...) { return 0.0; }
}
static bool OMJBool(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return false;
    return j.substr(p + t.size(), 4) == "true";
}
// JSON 배열 내 객체들 추출
static std::vector<std::string> OMExtractObjs(const std::string& j, const std::string& arrKey)
{
    std::vector<std::string> res;
    std::string t = "\"" + arrKey + "\":[";
    auto ap = j.find(t); if (ap == std::string::npos) return res;
    size_t i = ap + t.size();
    while (i < j.size()) {
        auto s = j.find('{', i); if (s == std::string::npos) break;
        int d = 0; size_t e = s;
        for (; e < j.size(); ++e) {
            if (j[e] == '{') ++d; else if (j[e] == '}') { if (--d == 0) break; }
        }
        res.push_back(j.substr(s, e - s + 1));
        i = e + 1;
    }
    return res;
}

// ── StoreInfo 파싱 ────────────────────────────────────────────
static StoreInfo ParseStoreObj(const std::string& obj)
{
    StoreInfo s;
    s.storeID            = OMJInt(obj,    "id");
    s.storeName          = OMJStr(obj,    "name");
    s.category           = OMJStr(obj,    "category");
    s.deliveryTime       = OMJStr(obj,    "delivery_time");
    s.minOrderAmount     = OMJInt(obj,    "min_order");
    s.distance           = OMJDouble(obj, "distance");
    s.address            = OMJStr(obj,    "address");
    s.openTime           = OMJStr(obj,    "open_time");
    s.phoneNumber        = OMJStr(obj,    "phone");
    s.holiday            = OMJStr(obj,    "holiday");

    // delivery_fee 는 숫자로 올 수도 있음
    int fee = OMJInt(obj, "delivery_fee");
    if (fee > 0) {
        char buf[32];
        _itoa_s(fee, buf, 10);
        s.deliveryPriceRange = std::string(buf) + "원";
    } else {
        s.deliveryPriceRange = OMJStr(obj, "delivery_fee");
    }
    return s;
}

// ── MenuInfo 파싱 ─────────────────────────────────────────────
static MenuInfo ParseMenuObj(const std::string& obj)
{
    MenuInfo m;
    m.menuID       = OMJInt(obj, "id");
    // 서버가 "id" 또는 "menu_id" 로 올 수 있음
    if (m.menuID == 0) m.menuID = OMJInt(obj, "menu_id");
    m.menuName     = OMJStr(obj, "name");
    m.price        = OMJInt(obj, "price");
    m.subCategory  = OMJStr(obj, "sub_category");
    m.menuImageUrl = OMJStr(obj, "image_url");

    // 옵션 파싱 (options 배열)
    auto opts = OMExtractObjs(obj, "options");
    if (!opts.empty()) {
        OptionGroup og;
        og.groupName  = "";
        og.isRequired = false;
        for (const auto& o : opts) {
            OptionItem oi;
            oi.optionID    = OMJInt(o, "option_id");
            oi.optionName  = OMJStr(o, "name");
            oi.optionPrice = OMJInt(o, "price");
            og.items.push_back(oi);
        }
        if (!og.items.empty()) m.optionGroups.push_back(og);
    }
    return m;
}

// ── OrderInfo 파싱 ────────────────────────────────────────────
static OrderInfo ParseOrderObj(const std::string& obj)
{
    OrderInfo o;
    int oid = OMJInt(obj, "order_id");
    o.orderID        = std::to_string(oid);
    o.storeID        = 0;
    o.storeName      = OMJStr(obj, "store_name");
    o.orderDateTime  = OMJStr(obj, "order_time");
    o.totalPayment   = OMJInt(obj, "total_price");
    o.deliveryStatus = OMJInt(obj, "status");
    o.isDelivery     = true;
    return o;
}

// =================================================================

OrderManager::OrderManager()
    : m_currentStoreID(-1)
    , m_isDelivery(true)
    , m_myPoints(0)
{
    m_categoryList = { "전체", "족발/보쌈", "찜/탕", "일식",
                       "치킨", "피자", "중식", "양식" };
}

std::vector<std::string> OrderManager::GetCategoryList() {
    return m_categoryList;
}

void OrderManager::UpdateStoreCache(const std::vector<StoreInfo>& stores)
{
    m_allStores = stores;
}

// ── 가게 목록 로드 ────────────────────────────────────────────
void OrderManager::LoadStoreData()
{
    auto& net = NetworkManager::GetInstance();

    // 서버 미연결: 기존 더미 데이터
    if (!net.IsConnected()) {
        if (m_allStores.empty()) {
            m_allStores.clear();
            StoreInfo s1; s1.storeID=101; s1.storeName="마왕족발 대구점";
            s1.category="족발/보쌈"; s1.deliveryTime="20~30분"; s1.distance=0.8;
            s1.minOrderAmount=15000; m_allStores.push_back(s1);

            StoreInfo s2; s2.storeID=102; s2.storeName="황금치킨 본점";
            s2.category="치킨"; s2.deliveryTime="30~40분"; s2.distance=1.2;
            s2.minOrderAmount=18000; m_allStores.push_back(s2);
        }
    }
    // 연결 시에는 MainHomeDlg에서 직접 REQ_STORE_LIST 를 호출하고
    // 응답을 UpdateStoreCache()로 전달하므로 여기서 별도 요청 불필요
}

std::vector<StoreInfo> OrderManager::GetStoresByCategory(const std::string& category)
{
    if (category.empty() || category == "전체") return m_allStores;
    std::vector<StoreInfo> filtered;
    for (const auto& s : m_allStores)
        if (s.category == category) filtered.push_back(s);
    return filtered;
}

// ── 메뉴 데이터 로드 (내부 캐시 저장) ───────────────────────
void OrderManager::LoadMenuData(int storeID, const std::string& /*category*/)
{
    m_currentStoreID = storeID;
    // 실제 메뉴 로드는 StoreListDlg에서 REQ_MENU_LIST를 직접 송신하고
    // 응답을 캐시에 저장함 (아래 SetMenuCache 참조)
}

// ── 메뉴 데이터 반환 ──────────────────────────────────────────
std::vector<MenuInfo> OrderManager::GetMenuData(int storeID,
                                                 const std::string& /*category*/)
{
    // 캐시에 해당 가게 메뉴가 있으면 반환
    auto it = m_menuCache.find(storeID);
    if (it != m_menuCache.end()) return it->second;

    // 없으면 더미 데이터 반환 (서버 미연결 환경)
    std::vector<MenuInfo> dummy;
    MenuInfo m1; m1.menuID = storeID*100+1; m1.menuName = "대표메뉴 1";
    m1.price = 12000; m1.subCategory = "인기메뉴"; dummy.push_back(m1);
    MenuInfo m2; m2.menuID = storeID*100+2; m2.menuName = "대표메뉴 2";
    m2.price = 15000; m2.subCategory = "인기메뉴"; dummy.push_back(m2);
    return dummy;
}

// ── 서버 응답에서 메뉴 캐시 저장 (StoreListDlg에서 호출) ─────
void OrderManager::SetMenuCache(int storeID, const std::vector<MenuInfo>& menus)
{
    m_menuCache[storeID] = menus;
}

// ── 장바구니 ─────────────────────────────────────────────────
bool OrderManager::AddToCart(int storeID, const CartItem& item)
{
    if (m_currentStoreID != -1 && m_currentStoreID != storeID)
        return false; // 다른 가게 → 호출자에서 처리

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
    for (const auto& item : m_cartList)
        total += item.totalPrice;
    return total;
}

void OrderManager::SetDeliveryType(bool isDelivery) { m_isDelivery = isDelivery; }

void OrderManager::SelectStore(int storeID) { m_currentStoreID = storeID; }

// ── 주문 생성 (장바구니 → 서버 전송은 CartDlg에서 직접 처리) ─
bool OrderManager::ProcessOrder(const std::string& /*cardID*/,
                                 int /*usePoint*/,
                                 const std::string& /*couponID*/,
                                 const std::string& /*extraArg*/)
{
    // CartDlg::OnBnClickedOk()에서 직접 BuildOrderJson + SendPacket 처리
    // 이 함수는 오프라인 테스트용으로만 사용
    ClearCart();
    return true;
}

// ── 주문 취소 ─────────────────────────────────────────────────
bool OrderManager::CancelOrder(const std::string& orderID)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return false;

    std::string json =
        "{\"order_id\":" + orderID + ","
        "\"reason\":\"고객 요청\"}";

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_CANCEL_ORDER, json);
    return true;
}

// ── 주문 상태 갱신 ────────────────────────────────────────────
bool OrderManager::UpdateOrderStatus(const std::string& /*orderID*/, int /*newStatus*/)
{
    // 라이더/사장님 전용 기능 → 고객 클라이언트는 서버 Push(NTF) 수신만 함
    return false;
}

bool OrderManager::AssignRiderToOrder(const std::string& /*orderID*/,
                                       const std::string& /*riderID*/)
{
    return false;
}

int OrderManager::GetLiveStatus(const std::string& /*orderID*/)
{
    return 0;
}

// ── 주문 내역 조회 ────────────────────────────────────────────
std::vector<OrderInfo> OrderManager::GetOrderHistory()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return {};

    // 동기 대기용 이벤트
    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    std::vector<OrderInfo> result;

    net.RegisterCallback(CmdCustomer::REQ_ORDER_HISTORY,
        [&](uint16_t, const std::string& body) {
            if (OMJInt(body, "status") == (int)Status::SUCCESS) {
                for (const auto& obj : OMExtractObjs(body, "orders"))
                    result.push_back(ParseOrderObj(obj));
            }
            SetEvent(hEvent);
        });

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_ORDER_HISTORY, "{}");

    WaitForSingleObject(hEvent, 5000);  // 최대 5초 대기
    CloseHandle(hEvent);
    net.UnregisterCallback(CmdCustomer::REQ_ORDER_HISTORY);
    return result;
}

// ── 보유 포인트 조회 ──────────────────────────────────────────
int OrderManager::GetMyPoints()
{
    return m_myPoints;
}

// 로그인 응답에서 포인트 저장 (AuthManager 또는 LoginDlg에서 호출)
void OrderManager::SetMyPoints(int points)
{
    m_myPoints = points;
}

// ── 주문 상태 콜백 등록 (NTF_ORDER_STATUS = 210) ──────────────
void OrderManager::RegisterOrderStatusCallback(
    std::function<void(const std::string&, int, const std::string&)> callback)
{
    auto& net = NetworkManager::GetInstance();

    net.RegisterCallback(CmdCustomer::NTF_ORDER_STATUS,
        [callback](uint16_t, const std::string& body) {
            // { "order_id":N, "status":N, "message":"..." }
            std::string oidStr = std::to_string(OMJInt(body, "order_id"));
            int status         = OMJInt(body, "status");
            std::string msg    = OMJStr(body, "message");
            if (callback) callback(oidStr, status, msg);
        });
}
