#pragma once
#include <vector>
#include <string>
#include "CartItem.h"
#include "OrderInfo.h"
#include "StoreInfo.h"

// [CUSTOMER/RIDER] 주문 및 배달 상태 관리 매니저 (Singleton)
class OrderManager
{
public:
    static OrderManager& GetInstance() {
        static OrderManager instance;
        return instance;
    }

    // 음식카테고리메뉴 목록을 UI에 전달하는 함수
    std::vector<std::string> GetCategoryList();

    // 전체 가게 목록 가져오기 및 카테고리 필터링
    void LoadStoreData(); // 초기 데이터 로드 (DB 대용)
    std::vector<StoreInfo> GetStoresByCategory(const std::string& category);

    // --- [고객 기능] ---
    void SetCurrentCategory(const std::string& category);
    void SelectStore(int storeID);

    // 장바구니 관련 (CUS-11, 12, 18)
    bool AddToCart(int storeID, const CartItem& item);
    void ClearCart();
    std::vector<CartItem> GetCartItems() const { return m_cartList; }
    int GetTotalAmount() const;
    void SetDeliveryType(bool isDelivery);

    // 주문 및 결제 (CUS-13)
    bool ProcessOrder(const std::string& cardID, int usePoint, const std::string& couponID);

    // 주문 취소 (CUS-17)
    bool CancelOrder(const std::string& orderID);

    // --- [라이더 및 상태 관리 기능] (추가/수정됨) ---
    // CUS-15: 실시간 배달 상태 업데이트 (0:접수전, 1:조리중, 2:배달중, 3:완료)
    // 사장님이나 라이더가 호출하여 상태를 변경함
    bool UpdateOrderStatus(const std::string& orderID, int newStatus);

    // 특정 주문에 라이더를 배정함
    bool AssignRiderToOrder(const std::string& orderID, const std::string& riderID);

    // 현재 상태 조회
    int GetLiveStatus(const std::string& orderID);
    std::vector<OrderInfo> GetOrderHistory();

    // --- [기타 포인트/쿠폰] ---
    int GetMyPoints();

private:
    OrderManager();
    ~OrderManager() {}

    std::vector<std::string> m_categoryList; // 음식카테고리메뉴 저장소
    std::vector<StoreInfo> m_allStores;
    std::vector<CartItem> m_cartList;
    int m_currentStoreID;
    bool m_isDelivery;

};