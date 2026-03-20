#pragma once
#include <afxdialogex.h>
#include "RoundButton.h"

class MyPageDlg : public CDialogEx {
    DECLARE_DYNAMIC(MyPageDlg)
public:
    MyPageDlg(CWnd* pParent = nullptr);
    virtual ~MyPageDlg();
    enum { IDD = IDD_MYPAGE_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH       m_hBrushBg       = nullptr;
    CRoundButton m_btnRiderName;
    CRoundButton m_btnSettlement;
    CRoundButton m_btnDriveTime;
    CRoundButton m_btnSettings;
    CRoundButton m_btnLogout;
    CRoundButton m_btnTodayHistory;

    afx_msg void    OnBtnTodayHistory();
    afx_msg void    OnBtnSettlement();
    afx_msg void    OnBtnDriveTime();
    afx_msg void    OnClickRiderName();
    afx_msg void    OnBtnSettings();
    afx_msg void    OnBtnLogout();
    afx_msg void    OnBtnBack();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void RequestTodaySummary();
    void ParseTodaySummary(const CString& payload);
};
