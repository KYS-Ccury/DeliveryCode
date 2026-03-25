#pragma once
#include <afxdialogex.h>
#include "resource.h"

class CLoginDlg : public CDialogEx
{
public:
    CLoginDlg(CWnd* pParent = nullptr);
    virtual ~CLoginDlg();

    enum { IDD = IDD_LOGIN_DLG };

protected:
    CEdit m_editId;
    CEdit m_editPw;

    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnBtnLogin();
    virtual void OnOK() override;

    DECLARE_MESSAGE_MAP()
};