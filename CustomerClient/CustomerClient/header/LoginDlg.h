#pragma once
#include "afxdialogex.h"

class LoginDlg : public CDialogEx
{
    DECLARE_DYNAMIC(LoginDlg)
public:
    LoginDlg(CWnd* pParent = nullptr);
    virtual ~LoginDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_LOGIN_DLG };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    DECLARE_MESSAGE_MAP()
};
