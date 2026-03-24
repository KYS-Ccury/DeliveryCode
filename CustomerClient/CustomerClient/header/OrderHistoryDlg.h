#pragma once
#include "afxdialogex.h"

class OrderHistoryDlg : public CDialogEx
{
    DECLARE_DYNAMIC(OrderHistoryDlg)
public:
    OrderHistoryDlg(CWnd* pParent = nullptr);
    virtual ~OrderHistoryDlg();
    CString m_strOrderNum;
    CString m_strShopName;
    int     m_nTotalAmount = 0;
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ORDERHISTORY_DLG };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBtnBack();
    afx_msg void OnBnClickedBtnChat();
    DECLARE_MESSAGE_MAP()
};
