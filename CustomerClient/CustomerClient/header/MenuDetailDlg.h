#pragma once
#include "afxdialogex.h"
#include "MenuInfo.h"
#include "CartItem.h"

class MenuDetailDlg : public CDialogEx
{
    DECLARE_DYNAMIC(MenuDetailDlg)
public:
    MenuDetailDlg(CWnd* pParent = nullptr);
    virtual ~MenuDetailDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MENUDETAIL_DLG };
#endif
    CString  m_strMenuName;
    MenuInfo m_menuInfo;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedBtnBack();
    afx_msg void OnBnClickedBtnCart();
    afx_msg void InsertOptionData();
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_listOptions;
};
