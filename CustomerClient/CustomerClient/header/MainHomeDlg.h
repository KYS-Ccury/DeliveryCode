#pragma once
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "StoreInfo.h"
#include "MyMenuPopup.h"

#include <vector>

#ifndef IDC_STATIC_CONN_STATUS
#define IDC_STATIC_CONN_STATUS      1903
#define IDC_BTN_MY_MYPAGE           1960
#define IDC_BTN_MY_PAYMENT          1961
#define IDC_BTN_MY_DELIVERY         1962
#define IDC_BTN_MY_ORDERHISTORY     1963
#endif

class MainHomeDlg : public CDialogEx
{
    DECLARE_DYNAMIC(MainHomeDlg)
public:
    MainHomeDlg(CWnd* pParent = nullptr);
    virtual ~MainHomeDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MAINHOME_DLG };
#endif
    std::vector<CString>   m_vecCategories;
    std::vector<StoreInfo> m_vecStoreCache;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();

    void RegisterNetworkCallback();
    void SendStoreListRequest(const CString& category);
    void RebuildStoreListUI(const std::vector<StoreInfo>& stores);
    void UpdateStoreListUI(CString categoryName);
    void UpdateConnStatusUI();

    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStoreListResponse(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedButton1();                  // 장바구니
    afx_msg void    OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult);

    // ── 하단 4버튼 ─────────────────────────────────────────
    afx_msg void    OnBnClickedBtnMypage();       // 👤 My 페이지
    afx_msg LRESULT OnMyMenuSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedBtnPayment();      // 💳 결제수단
    afx_msg void    OnBnClickedBtnDelivery();     // 🛵 배달현황
    afx_msg void    OnBnClickedBtnOrderHistory(); // 📋 주문내역

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl   m_listStore;
    CScrollMenu m_wndScrollMenu;
    CBrush      m_brushBack;
    CBrush      m_brushWhite;
    bool        m_bLastConnState = false;

    static const UINT TIMER_CONN_CHECK = 1;

    MyMenuPopup* m_pMyMenuPopup = nullptr;
};
