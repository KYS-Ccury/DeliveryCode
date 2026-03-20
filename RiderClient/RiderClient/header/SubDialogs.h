#pragma once
#include <afxdialogex.h>
#include <afxcmn.h>

// ======================================================
//  SettingsDlg
// ======================================================
class SettingsDlg : public CDialogEx {
    DECLARE_DYNAMIC(SettingsDlg)
public:
    SettingsDlg(CWnd* pParent = nullptr);
    virtual ~SettingsDlg();
    enum { IDD = IDD_SETTINGS_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    CString m_strRegionNews;
    CString m_strDispatchType;

    afx_msg void OnBtnRegionNews();
    afx_msg void OnBtnDispatchType();
    afx_msg void OnBtnSave();

    void LoadSettings();
    void SaveSettings();
};

// ======================================================
//  SettlementDlg
// ======================================================
class SettlementDlg : public CDialogEx {
    DECLARE_DYNAMIC(SettlementDlg)
public:
    SettlementDlg(CWnd* pParent = nullptr);
    virtual ~SettlementDlg();
    enum { IDD = IDD_SETTLEMENT_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_listSettlement;

    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void RequestSettlement();
    void ParseAndFillList(const CString& payload);
};

// ======================================================
//  DriveTimeDlg
// ======================================================
class DriveTimeDlg : public CDialogEx {
    DECLARE_DYNAMIC(DriveTimeDlg)
public:
    DriveTimeDlg(CWnd* pParent = nullptr);
    virtual ~DriveTimeDlg();
    enum { IDD = IDD_DRIVETIME_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    int   m_nTodayBaseSec = 0;
    DWORD m_dwOpenTime    = 0;

    afx_msg void OnTimer(UINT_PTR nIDEvent);

    void    UpdateDriveTimeUI();
    CString FormatSeconds(int totalSec);
    void    CalcWeekRange(CString& outRange);
};

// ======================================================
//  TodayHistoryDlg
// ======================================================
class TodayHistoryDlg : public CDialogEx {
    DECLARE_DYNAMIC(TodayHistoryDlg)
public:
    TodayHistoryDlg(CWnd* pParent = nullptr);
    virtual ~TodayHistoryDlg();
    enum { IDD = IDD_TODAY_HISTORY_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_listHistory;

    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void RequestTodayHistory();
    void ParseAndFill(const CString& payload);
};
