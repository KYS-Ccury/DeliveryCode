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
    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnCart();
    afx_msg void    OnBnClickedBtnStoreInfo();
    afx_msg void    OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
    void UpdateMenuListUI(CString subCategory);
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl             m_listMenu;
    CScrollMenu           m_wndScrollMenu;
    std::vector<CString>  m_vecSubCategories;
    std::vector<MenuInfo> m_vecMenuCache;
};
