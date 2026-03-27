#pragma once
// ================================================================
//  MainHomeDlg.h
//
//  [수정] 이미지 TCP 수신 관련 추가
//    - WM_IMAGE_RESPONSE 메시지
//    - OnImageResponse 핸들러
//    - RequestStoreImages() 메서드
//    - m_imageUrlToIndex 맵 (image_url → ImageList 슬롯 인덱스)
// ================================================================
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "StoreInfo.h"
#include "MyMenuPopup.h"
#include <vector>
#include <unordered_map>
#include <string>

#ifndef IDC_STATIC_CONN_STATUS
#define IDC_STATIC_CONN_STATUS      1903
#define IDC_BTN_MY_MYPAGE           1960
#define IDC_BTN_MY_PAYMENT          1961
#define IDC_BTN_MY_DELIVERY         1962
#define IDC_BTN_MY_ORDERHISTORY     1963
#endif

#define WM_ADDR_LIST_RESPONSE   (WM_USER + 115)
#define WM_ADDR_SAVE_RESPONSE   (WM_USER + 116)
// ★ 이미지 응답 메시지
#define WM_IMAGE_RESPONSE       (WM_USER + 117)

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

    // ★ 이미지 비동기 요청
    void RequestStoreImages(const std::vector<StoreInfo>& stores);

    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStoreListResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddrListResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddrSaveResponse(WPARAM wParam, LPARAM lParam);
    // ★ 이미지 응답 핸들러
    afx_msg LRESULT OnImageResponse(WPARAM wParam, LPARAM lParam);

    afx_msg void    OnBnClickedButton1();
    afx_msg void    OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedBtnMypage();
    afx_msg LRESULT OnMyMenuSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedBtnPayment();
    afx_msg void    OnBnClickedBtnDelivery();
    afx_msg void    OnBnClickedBtnOrderHistory();
    afx_msg void    OnBnClickedBtnAddr();

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl   m_listStore;
    CScrollMenu m_wndScrollMenu;
    CBrush      m_brushBack;
    CBrush      m_brushWhite;
    bool        m_bLastConnState = false;
    static const UINT TIMER_CONN_CHECK = 1;
    MyMenuPopup* m_pMyMenuPopup = nullptr;
    CImageList   m_imgListStore;

    // ★ image_url → ImageList 슬롯 인덱스 (비동기 이미지 교체용)
    std::unordered_map<std::string, int> m_imageUrlToIndex;

    HBITMAP LoadImageFromServer(const CString& relPath); // 하위 호환
    void    ResizeBitmapTo(HBITMAP& hBmp, int w, int h); // 하위 호환
};
