#pragma once
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "MenuInfo.h"
#include "StoreInfo.h"
#include <vector>

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

    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnCart();
    afx_msg void    OnBnClickedBtnStoreInfo();  // 팝업: 가게정보 / 리뷰
    afx_msg void    OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMenuListResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl             m_listMenu;
    CScrollMenu           m_wndScrollMenu;
    std::vector<CString>  m_vecSubCategories;
    std::vector<MenuInfo> m_vecMenuCache;

    // 메뉴 썸네일 ImageList
    CImageList m_imgListMenu;

    HBITMAP LoadMenuImage(const CString& relPath);
    void    ResizeBitmapTo(HBITMAP& hBmp, int w, int h);
};
