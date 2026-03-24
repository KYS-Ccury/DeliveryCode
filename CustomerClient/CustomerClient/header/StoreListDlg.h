#pragma once
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "MenuInfo.h"
#include "StoreInfo.h"
#include <vector>

// ================================================================
//  StoreListDlg.h  ─  가게 상세 / 메뉴 목록 화면
//  [추가]
//    RegisterMenuCallback()    : ReceiveLoop 콜백 등록
//    SendMenuListRequest()     : 서버에 메뉴 목록 요청
//    OnMenuListResponse()      : WM_USER+120 핸들러
//    RebuildMenuListUI()       : 서버 응답 파싱 후 리스트 갱신
// ================================================================
class StoreListDlg : public CDialogEx
{
    DECLARE_DYNAMIC(StoreListDlg)
public:
    StoreListDlg(CWnd* pParent = nullptr);
    virtual ~StoreListDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_STORELIST_DLG };
#endif

    CString   m_strStoreName;
    StoreInfo m_storeInfo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    // 서버 연동
    void RegisterMenuCallback();
    void SendMenuListRequest(const CString& subCategory);
    void RebuildMenuListUI(const std::vector<MenuInfo>& menus, const CString& filter);
    void UpdateMenuListUI(CString subCategory);  // 로컬 폴백

    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnCart();
    afx_msg void    OnBnClickedBtnStoreInfo();
    afx_msg void    OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMenuListResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl             m_listMenu;
    CScrollMenu           m_wndScrollMenu;
    std::vector<CString>  m_vecSubCategories;
    std::vector<MenuInfo> m_vecMenuCache;   // 서버에서 받은 전체 메뉴 캐시
};
