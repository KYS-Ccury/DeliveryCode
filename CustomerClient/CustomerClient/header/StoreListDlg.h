#pragma once
// ================================================================
//  StoreListDlg.h
//
//  [수정]
//  - 가게 로고: OnInitDialog에서 TCP(217)로 로고 이미지 요청
//  - 메뉴 이미지: RebuildMenuListUI에서 TCP(217)로 메뉴 이미지 요청
//  - WM_LOGO_RESPONSE, WM_MENU_IMG_RESPONSE 메시지 추가
// ================================================================
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "MenuInfo.h"
#include "StoreInfo.h"
#include <vector>
#include <unordered_map>
#include <string>

#define WM_LOGO_RESPONSE     (WM_USER + 130)
#define WM_MENU_IMG_RESPONSE (WM_USER + 131)

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

    void RegisterMenuCallback();
    void SendMenuListRequest(const CString& subCategory);
    void RebuildMenuListUI(const std::vector<MenuInfo>& menus, const CString& filter);
    void UpdateMenuListUI(CString subCategory);

    // ★ 이미지 TCP 요청
    void RequestLogoImage();
    void RequestMenuImages(const std::vector<MenuInfo>& menus, const CString& filter);

    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnCart();
    afx_msg void    OnBnClickedBtnStoreInfo();
    afx_msg void    OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMenuListResponse(WPARAM wParam, LPARAM lParam);
    // ★ 이미지 응답 핸들러
    afx_msg LRESULT OnLogoResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMenuImgResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl             m_listMenu;
    CScrollMenu           m_wndScrollMenu;
    std::vector<CString>  m_vecSubCategories;
    std::vector<MenuInfo> m_vecMenuCache;
    CImageList            m_imgListMenu;
    CStatic               m_storeLogoCtrl; // IDC_STATIC_STORE_IMG 로고 표시 컨트롤

    // ★ 메뉴 image_url → ImageList 슬롯 인덱스
    std::unordered_map<std::string, int> m_menuImgUrlToIndex;

    HBITMAP LoadMenuImage(const CString& relPath);
    void    ResizeBitmapTo(HBITMAP& hBmp, int w, int h);
};
