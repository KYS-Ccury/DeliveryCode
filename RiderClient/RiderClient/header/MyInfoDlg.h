#pragma once
#include <afxdialogex.h>

// ======================================================
//  MyInfoDlg
// ======================================================
class MyInfoDlg : public CDialogEx {
    DECLARE_DYNAMIC(MyInfoDlg)
public:
    MyInfoDlg(CWnd* pParent = nullptr);
    virtual ~MyInfoDlg();
    enum { IDD = IDD_MYINFO_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH m_hBrushBg = nullptr;
    afx_msg void    OnBtnVehicle();
    afx_msg void    OnBtnChangePw();
    afx_msg void    OnBtnChangeAcct();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
};

// ======================================================
//  ChangePwDlg
// ======================================================
class ChangePwDlg : public CDialogEx {
    DECLARE_DYNAMIC(ChangePwDlg)
public:
    ChangePwDlg(CWnd* pParent = nullptr);
    virtual ~ChangePwDlg();
    enum { IDD = IDD_CHANGE_PW_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH m_hBrushBg = nullptr;
    CEdit m_editCurPw;
    CEdit m_editNewPw;
    CEdit m_editNewPwConfirm;

    afx_msg void    OnBtnConfirm();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
};

// ======================================================
//  ChangeAcctDlg
// ======================================================
class ChangeAcctDlg : public CDialogEx {
    DECLARE_DYNAMIC(ChangeAcctDlg)
public:
    ChangeAcctDlg(CWnd* pParent = nullptr);
    virtual ~ChangeAcctDlg();
    enum { IDD = IDD_CHANGE_ACCT_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH m_hBrushBg = nullptr;
    CEdit m_editBank;
    CEdit m_editHolder;
    CEdit m_editAccount;
    bool  m_bEditMode = false;

    afx_msg void    OnBtnChange();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void ShowEditMode(bool bEdit);
    void DoSaveAcct();
};
