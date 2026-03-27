#pragma once
// ================================================================
//  MainHomeDlg.h
//
//  [수정] 주소 목록 서버 응답 처리 추가
//    - WM_ADDR_LIST_RESPONSE 메시지 추가
//    - OnAddrListResponse 핸들러 선언 추가
// ================================================================
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

// ★ 주소 목록 응답 메시지
#define WM_ADDR_LIST_RESPONSE   (WM_USER + 115)
#define WM_ADDR_SAVE_RESPONSE   (WM_USER + 116)

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
    void UpdateAddrLabel();

    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStoreListResponse(WPARAM wParam, LPARAM lParam);
    // ★ 주소 목록 응답 핸들러
    afx_msg LRESULT OnAddrListResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddrSaveResponse(WPARAM wParam, LPARAM lParam);

    afx_msg void    OnBnClickedButton1();                  // 장바구니
    afx_msg void    OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult);

    // ── 하단 4버튼 ─────────────────────────────────────────
    afx_msg void    OnBnClickedBtnMypage();       // 👤 My 페이지
    afx_msg LRESULT OnMyMenuSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedBtnPayment();      // 💳 결제수단
    afx_msg void    OnBnClickedBtnDelivery();     // 🛵 배달현황
    afx_msg void    OnBnClickedBtnOrderHistory(); // 📋 주문내역
    afx_msg void    OnBnClickedBtnAddr();         // 📍 주소 관리

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl   m_listStore;
    CScrollMenu m_wndScrollMenu;
    CBrush      m_brushBack;
    CBrush      m_brushWhite;
    bool        m_bLastConnState = false;

    static const UINT TIMER_CONN_CHECK = 1;

    MyMenuPopup* m_pMyMenuPopup = nullptr;

    // 가게 목록 썸네일 ImageList
    CImageList  m_imgListStore;

    HBITMAP LoadImageFromServer(const CString& relPath);
    void    ResizeBitmapTo(HBITMAP& hBmp, int w, int h);
};
