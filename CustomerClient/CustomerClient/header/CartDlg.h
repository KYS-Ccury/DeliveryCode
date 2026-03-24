#pragma once
#include <vector>
#include "afxdialogex.h"
#include "CartItem.h"

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
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedBtnBack();
    afx_msg void OnBnClickedBtnEditOption();
    DECLARE_MESSAGE_MAP()
};
