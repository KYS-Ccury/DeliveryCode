#pragma once
#include <afxdialogex.h>

class LoginDlg : public CDialogEx {
    DECLARE_DYNAMIC(LoginDlg)
public:
    LoginDlg(CWnd* pParent = nullptr);
    virtual ~LoginDlg();
    enum { IDD = IDD_LOGIN_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    CEdit   m_editId;
    CEdit   m_editPw;
    CButton m_checkSaveId;

    afx_msg void    OnBtnLogin();
    afx_msg void    OnBtnRegister();
    afx_msg void    OnBtnFindId();
    afx_msg void    OnBtnFindPw();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void LoadSavedId();
    void SaveId(const CString& strId);
    void OpenMainDlg();
};
