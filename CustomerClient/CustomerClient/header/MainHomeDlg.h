#pragma once
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "StoreInfo.h"
#include <vector>

// ================================================================
//  MainHomeDlg.h  ─  메인 홈 화면
//  [추가]
//    SendStoreListRequest() : 서버에 가게 목록 요청
//    RebuildStoreListUI()   : 파싱된 StoreInfo 벡터로 리스트 갱신
//    OnStoreListResponse()  : WM_USER+110 메시지 핸들러
//    RegisterNetworkCallback() : ReceiveLoop 콜백 등록
// ================================================================
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
    std::vector<StoreInfo> m_vecStoreCache;  // 서버 응답 캐시

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    // 서버 연동
    void RegisterNetworkCallback();
    void SendStoreListRequest(const CString& category);
    void RebuildStoreListUI(const std::vector<StoreInfo>& stores);
    void UpdateStoreListUI(CString categoryName);  // 로컬 폴백

    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStoreListResponse(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedButton1();
    afx_msg void    OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedBtnOrderHistory();

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl   m_listStore;
    CScrollMenu m_wndScrollMenu;
    CBrush      m_brushBack;
    CBrush      m_brushWhite;
};
