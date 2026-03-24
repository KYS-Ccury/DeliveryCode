#pragma once
#include "afxdialogex.h"
#include "ScrollMenu.h"
#include "StoreInfo.h"
#include <vector>

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
    void UpdateStoreListUI(CString categoryName);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);
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
