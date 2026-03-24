#pragma once
#include <vector>
#include <atomic>
#include "afxdialogex.h"
#include "CartItem.h"

// ================================================================
//  CartDlg.h  ─  장바구니 / 주문 요청 화면 (수정본)
//  resource.h 에 정의된 IDC만 사용:
//    IDC_LIST_CART / IDC_BTN_BACK / IDC_BTN_EDIT_OPTION
//    IDC_RADIO_DELIVERY / IDC_RADIO_PICKUP
//    IDC_STATIC_ORDER_PRICE / IDC_STATIC_DELIVERY_FEE / IDC_STATIC_TOTAL_PRICE
// ================================================================
class CartDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CartDlg)
public:
    CartDlg(CWnd* pParent = nullptr);
    virtual ~CartDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_CART_DLG };
#endif

    std::vector<CartItem> m_vecCart;
    CListCtrl             m_listCart;
    void UpdateCartUI();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnEditOption();
    afx_msg void    OnBnClickedRadioDelivery();
    afx_msg void    OnBnClickedRadioPickup();
    afx_msg LRESULT OnOrderResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    bool              m_bDelivery;   // true=배달, false=포장
    std::atomic<bool> m_bWaiting;    // 주문 처리 중 중복 방지
};
