#pragma once
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "CartItem.h"
#include "OrderInfo.h"
#include "StoreInfo.h"
#include "MenuInfo.h"

// ================================================================
//  OrderManager.h  ─  주문/가게/메뉴 매니저 (완성판)
//
//  [변경사항]
//  - SetMenuCache(storeID, menus): 서버 응답 메뉴를 캐시에 저장
//  - SetMyPoints(points):          로그인 시 포인트 저장
//  - m_menuCache, m_myPoints 멤버 추가
// ================================================================
class OrderManager
{
public:
    static OrderManager& GetInstance() {
        static OrderManager instance;
        return instance;
    }

    // 카테고리 목록
    std::vector<std::string> GetCategoryList();

    // 가게 목록
    void LoadStoreData();
    void UpdateStoreCache(const std::vector<StoreInfo>& stores);
    std::vector<StoreInfo> GetStoresByCategory(const std::string& category);

    // ── 고객 기능 ─────────────────────────────────────────────
    void SetCurrentCategory(const std::string& category) {}
    void SelectStore(int storeID);

    // 장바구니
    bool AddToCart(int storeID, const CartItem& item);
    void ClearCart();
    std::vector<CartItem> GetCartItems() const { return m_cartList; }
    int  GetTotalAmount() const;
    void SetDeliveryType(bool isDelivery);

    // 주문
    void RegisterOrderStatusCallback(
        std::function<void(const std::string&, int, const std::string&)> callback);

    // 메뉴 로드 / 조회
    void LoadMenuData(int storeID, const std::string& category = "");
    std::vector<MenuInfo> GetMenuData(int storeID, const std::string& category = "");

    // ★ 서버 응답 메뉴를 캐시에 저장 (StoreListDlg에서 호출)
    void SetMenuCache(int storeID, const std::vector<MenuInfo>& menus);

    // 주문 처리 (오프라인 테스트용)
    bool ProcessOrder(const std::string& cardID, int usePoint,
                      const std::string& couponID, const std::string& extraArg = "");

    // 주문 취소
    bool CancelOrder(const std::string& orderID);

    // 라이더/사장 전용 (고객 클라이언트에서는 미사용)
    bool UpdateOrderStatus(const std::string& orderID, int newStatus);
    bool AssignRiderToOrder(const std::string& orderID, const std::string& riderID);
    int  GetLiveStatus(const std::string& orderID);

    // 주문 내역 조회
    std::vector<OrderInfo> GetOrderHistory();

    // 현재 선택된 가게 ID
    int GetCurrentStoreID() const { return m_currentStoreID; }

    // 포인트
    int  GetMyPoints();
    void SetMyPoints(int points);   // ★ 로그인 응답에서 포인트 저장

    // 배달비
    int GetDeliveryFeeByStore(int storeID);

private:
    OrderManager();
    ~OrderManager() {}

    std::vector<std::string>          m_categoryList;
    std::vector<StoreInfo>            m_allStores;
    std::vector<CartItem>             m_cartList;
    std::map<int, std::vector<MenuInfo>> m_menuCache;  // ★ storeID → 메뉴 목록
    int                               m_currentStoreID;
    bool                              m_isDelivery;
    int                               m_myPoints;      // ★ 보유 포인트
};
