#pragma once
#include <afxdialogex.h>

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
private:
    afx_msg void    OnBtnTodayHistory();
    afx_msg void    OnBtnSettlement();
    afx_msg void    OnBtnDriveTime();
    afx_msg void    OnBtnMyInfo();
    afx_msg void    OnBtnSettings();
    afx_msg void    OnBtnLogout();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void RequestTodaySummary();
    void ParseTodaySummary(const CString& payload);
};
