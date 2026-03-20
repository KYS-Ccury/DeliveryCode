#pragma once
#include <afxdialogex.h>
#include <afxcmn.h>
#include "RoundButton.h"

class DispatchDlg : public CDialogEx {
    DECLARE_DYNAMIC(DispatchDlg)
public:
    DispatchDlg(const CString& pushData, CWnd* pParent = nullptr);
    virtual ~DispatchDlg();
    enum { IDD = IDD_DISPATCH_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnCancel() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH        m_hBrushBg   = nullptr;
    CRoundButton  m_btnAccept;
    CRoundButton  m_btnReject;
    CString       m_pushData;
    CProgressCtrl m_progressTimer;
    int     m_orderId     = 0;
    CString m_storeName;
    CString m_pickupAddr;
    CString m_destAddr;
    int     m_deliveryFee = 0;
    int     m_remainSec   = 60;

    afx_msg void OnBtnAccept();
    afx_msg void OnBtnReject();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    void ParsePushData();
    void UpdateTimerUI();
    void DoReject(bool bTimeout);
};
