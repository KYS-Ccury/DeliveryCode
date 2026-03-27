#pragma once
// ================================================================
//  PointDlg.h  ─  포인트 확인 다이얼로그
//
//  [연동 프로토콜]
//  ▶ 포인트 조회
//    REQ: { "token":"..." }
//    RES: { "status":2000, "points":1500,
//           "history":[
//             {"date":"2024-05-22","desc":"주문 적립","amount":300},
//             {"date":"2024-05-20","desc":"포인트 사용","amount":-500}
//           ]}
//
//  resource.h 사용 IDC:
//    IDD_POINT_DLG               159
//    IDC_STATIC_MY_POINT_TOTAL   2210
//    IDC_LIST_POINT_HISTORY      2211
//    IDC_BTN_BACK                1400  (공통)
// ================================================================
#include "afxdialogex.h"

#ifndef IDD_POINT_DLG
#define IDD_POINT_DLG               159
#define IDC_STATIC_MY_POINT_TOTAL   2210
#define IDC_LIST_POINT_HISTORY      2211
#endif

#define WM_POINT_RESPONSE (WM_USER + 220)

class PointDlg : public CDialogEx
{
    DECLARE_DYNAMIC(PointDlg)
public:
    PointDlg(CWnd* pParent = nullptr);
    virtual ~PointDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_POINT_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnBack();
    afx_msg LRESULT OnPointResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl m_listHistory;

    void RequestPoint();
};
