#pragma once
#include <vector>
#include <string>
#include <functional>
#include "CartItem.h"
#include "OrderInfo.h"
#include "StoreInfo.h"

// ================================================================
//  OrderManager.h  ─  주문 및 배달 상태 관리 (Singleton)
//  [추가]
//    UpdateStoreCache()       : 서버 응답으로 받은 가게 목록 저장
//    m_orderStatusCallback    : 실시간 주문 상태 변경 콜백
// ================================================================
class OrderManager
{
public:
    static OrderManager& GetInstance() {
        static OrderManager instance;
        return instance;
    }

    // 카테고리 / 가게
    std::vector<std::string>  GetCategoryList();
    void                      LoadStoreData();
    void                      UpdateStoreCache(const std::vector<StoreInfo>& stores); // ★ 신규
    std::vector<StoreInfo>    GetStoresByCategory(const std::string& category);
    void                      SetCurrentCategory(const std::string& category) {}
    void                      SelectStore(int storeID);
    int                       GetCurrentStoreID() const { return m_currentStoreID; }

    // 장바구니
    bool                      AddToCart(int storeID, const CartItem& item);
    void                      ClearCart();
    std::vector<CartItem>     GetCartItems() const { return m_cartList; }
    int                       GetTotalAmount() const;
    void                      SetDeliveryType(bool isDelivery);

    // 주문 / 결제 (로컬 폴백용)
    bool ProcessOrder(const std::string& cardID, int usePoint,
                      const std::string& couponID, const std::string& extraArg = "");
    bool CancelOrder(const std::string& orderID);

    // 주문 상태
    void RegisterOrderStatusCallback(
        std::function<void(const std::string&, int, const std::string&)> callback);
    bool                       UpdateOrderStatus(const std::string& orderID, int newStatus);
    bool                       AssignRiderToOrder(const std::string& orderID,
                                                  const std::string& riderID);
    int                        GetLiveStatus(const std::string& orderID);
    std::vector<OrderInfo>     GetOrderHistory();

    // 메뉴
    void                       LoadMenuData(int storeID, const std::string& category = "");
    std::vector<MenuInfo>      GetMenuData(int storeID, const std::string& category = "");

    // 기타
    int GetMyPoints();

private:
    OrderManager();
    ~OrderManager() {}

    std::vector<std::string>  m_categoryList;
    std::vector<StoreInfo>    m_allStores;
    std::vector<CartItem>     m_cartList;
    int                       m_currentStoreID = -1;
    bool                      m_isDelivery     = true;

    std::function<void(const std::string&, int, const std::string&)> m_orderStatusCallback;
};
