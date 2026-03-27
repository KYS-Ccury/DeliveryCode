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
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH    m_hBrushBg = nullptr;
    CEdit     m_editId;
    CEdit     m_editPw;
    CEdit     m_editPwConfirm;
    CEdit     m_editPhone;      // IDC_EDIT_PHONE (1008) - exists in resource.h
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
