#pragma once
#include <afxdialogex.h>
#include <afxcmn.h>
#include "RoundButton.h"

enum class DeliveryStep {
    IDLE = 0,
    ONLINE,
    PICKUP_MOVING,
    STORE_ARRIVED,
    PICKED_UP,
    DELIVERING,
    DEST_ARRIVED,
    DELIVERED
};

class MainDlg : public CDialogEx {
    DECLARE_DYNAMIC(MainDlg)
public:
    MainDlg(CWnd* pParent = nullptr);
    virtual ~MainDlg();
    enum { IDD = IDD_MAIN_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH       m_hBrushBg     = nullptr;
    CStatic      m_staticMap;
    CButton      m_checkNewDispatch;
    CRoundButton m_btnStartDrive;
    CRoundButton m_btnStepAction;
    CRoundButton m_btnMyPage;
    CRoundButton m_btnDeliveryList;

    DeliveryStep m_step        = DeliveryStep::IDLE;
    bool         m_bDriving    = false;
    ULONGLONG    m_dwStartTime = 0;

    afx_msg void    OnClose();
    afx_msg void    OnBtnHelp();
    afx_msg void    OnBtnStartDrive();
    afx_msg void    OnBtnMyPage();
    afx_msg void    OnBtnDeliveryList();
    afx_msg void    OnBtnStepAction();
    afx_msg void    OnCheckNewDispatch();
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnPaint();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
    afx_msg LRESULT OnDispatchPush(WPARAM w, LPARAM l);
    afx_msg LRESULT OnServerDisconn(WPARAM w, LPARAM l);

    void    SetStep(DeliveryStep s);
    CString StepToString(DeliveryStep s);
    void    SendStatusToServer(const CString& statusStr);
    void    UpdateStepUI();
    void    DrawSimpleMap(CDC* pDC, const CRect& rect);
};
