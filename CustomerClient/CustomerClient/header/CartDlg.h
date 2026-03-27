#pragma once
#include <vector>
#include <atomic>
#include "afxdialogex.h"
#include "CartItem.h"

// ================================================================
//  CartDlg.h  ─  장바구니 / 주문하기 (완성판)
//
//  [변경사항]
//  - IDC 임시 정의 확장 (배달주소, 포인트, 쿠폰)
//  - m_strDeliveryRequest 기존 유지
// ================================================================

#ifndef IDC_EDIT_DELIVERY_REQUEST
#define IDC_EDIT_DELIVERY_REQUEST   1970
#define IDC_STATIC_REQUEST_LABEL    1971
#endif
#ifndef IDC_EDIT_DELIVERY_ADDR
#define IDC_EDIT_DELIVERY_ADDR      2020
#define IDC_STATIC_ADDR_LABEL       2021
#define IDC_EDIT_USE_POINT          2022
#define IDC_STATIC_MY_POINT         2023
#define IDC_EDIT_COUPON_ID          2024
#endif

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
    afx_msg void OnBnClickedBtnAddrChange();
    afx_msg void OnEnChangeEditUsePoint();
    DECLARE_MESSAGE_MAP()

private:
    bool              m_bDelivery{ true };
    std::atomic<bool> m_bWaiting{ false };
    CString           m_strDeliveryRequest;

    std::vector<int>  m_vecCardIDs;
    std::string       m_strLastAddr;    // 주문 시 배달 주소 캐시
    int               m_nLastUsePoint{ 0 }; // 주문 시 포인트 사용액 캐시

    // 1. 포인트 관련 변수 추가
    int m_nUsePoint = 0;      // 실시간 입력 중인 포인트 (UpdateCartUI용)


    void LoadPaymentCards();
};
