#pragma once
#include <afxdialogex.h>

class PickupCodeDlg : public CDialogEx {
    DECLARE_DYNAMIC(PickupCodeDlg)
public:
    PickupCodeDlg(CWnd* pParent = nullptr);
    virtual ~PickupCodeDlg();
    enum { IDD = IDD_PICKUP_CODE_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH m_hBrushBg = nullptr;
    CString m_strInput;
    bool    m_bQRMode = false;

    afx_msg void OnBtnNumberPickup();
    afx_msg void OnBtnQRPickup();
    afx_msg void OnBtnConfirmCode();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    // 소프트 키패드
    afx_msg void OnKey0();
    afx_msg void OnKey1();
    afx_msg void OnKey2();
    afx_msg void OnKey3();
    afx_msg void OnKey4();
    afx_msg void OnKey5();
    afx_msg void OnKey6();
    afx_msg void OnKey7();
    afx_msg void OnKey8();
    afx_msg void OnKey9();
    afx_msg void OnKeyA();
    afx_msg void OnKeyDel();

    void PressKey(TCHAR ch);
    void UpdateCodeDisplay();
    void ShowQRView(bool bShow);
};
