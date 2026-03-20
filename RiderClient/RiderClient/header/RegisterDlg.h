#pragma once
#include <afxdialogex.h>

class RegisterDlg : public CDialogEx {
    DECLARE_DYNAMIC(RegisterDlg)
public:
    RegisterDlg(CWnd* pParent = nullptr);
    virtual ~RegisterDlg();
    enum { IDD = IDD_REGISTER_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    CEdit     m_editId;
    CEdit     m_editPw;
    CEdit     m_editPwConfirm;
    CEdit     m_editRegion;
    CComboBox m_cmbVehicle;

    int  m_nStep      = 1;
    bool m_bIdChecked = false;

    afx_msg void    OnBtnCheckId();
    afx_msg void    OnBtnNext();
    afx_msg void    OnBtnPrev();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void ShowStep(int step);
    bool ValidateStep1();
    bool ValidateStep2();
    void DoRegister();
    void ShowDlgItem(UINT nID, BOOL bShow);
};
