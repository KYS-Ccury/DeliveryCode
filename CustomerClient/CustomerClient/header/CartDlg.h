#pragma once
#include <vector>
#include <atomic>
#include "afxdialogex.h"
#include "CartItem.h"

#ifndef IDC_EDIT_DELIVERY_REQUEST
#define IDC_EDIT_DELIVERY_REQUEST   1970
#define IDC_STATIC_REQUEST_LABEL    1971
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
    DECLARE_MESSAGE_MAP()

private:
    bool              m_bDelivery{ true };
    std::atomic<bool> m_bWaiting{ false };
    CString           m_strDeliveryRequest;  // ★ 배달원 요청사항
};
