#pragma once
#include <afxdialogex.h>

class PrepareDeliveryDlg : public CDialogEx {
    DECLARE_DYNAMIC(PrepareDeliveryDlg)
public:
    PrepareDeliveryDlg(CWnd* pParent = nullptr);
    virtual ~PrepareDeliveryDlg();
    enum { IDD = IDD_PREPARE_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH m_hBrushBg = nullptr;
    CEdit m_editResidentFront;
    CEdit m_editResidentBack;
    CEdit m_editBank;
    CEdit m_editAccountHolder;
    CEdit m_editAccountNumber;
    int   m_nStep = 1;

    afx_msg void    OnBtnNext();
    afx_msg void    OnBtnPrev();
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void ShowStep(int step);
    bool ValidateStep1();
    bool ValidateStep2();
    void SubmitAll();
};
