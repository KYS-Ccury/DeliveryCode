#pragma once
// ================================================================
//  MyInfoDlg.h  ─  개인정보 확인 다이얼로그
//
//  [표시 항목]
//  - 아이디 / 이름 / 전화번호 / 주소 / 등급
//
//  [연동 프로토콜]
//  ▶ 내 정보 조회
//    REQ: { "token":"..." }
//    RES: { "status":2000,
//           "user_id":"hong123", "name":"홍길동",
//           "phone":"010-1234-5678", "address":"서울시 ...",
//           "role":"일반회원", "grade":"브론즈" }
//
//  resource.h 사용 IDC:
//    IDD_MYINFO_DLG              161
//    IDC_EDIT_MI_ID              2300
//    IDC_EDIT_MI_NAME            2301
//    IDC_EDIT_MI_PHONE           2302
//    IDC_EDIT_MI_ADDR            2303
//    IDC_EDIT_MI_GRADE           2304
//    IDC_BTN_BACK                1400  (공통)
// ================================================================
#include "afxdialogex.h"
#include <string>

//#ifndef IDD_MYINFO_DLG
//#define IDD_MYINFO_DLG      161
//#define IDC_EDIT_MI_ID      2300
//#define IDC_EDIT_MI_NAME    2301
//#define IDC_EDIT_MI_PHONE   2302
//#define IDC_EDIT_MI_ADDR    2303
//#define IDC_EDIT_MI_GRADE   2304
//#endif

#define WM_MYINFO_RESPONSE (WM_USER + 230)

class MyInfoDlg : public CDialogEx
{
    DECLARE_DYNAMIC(MyInfoDlg)
public:
    MyInfoDlg(CWnd* pParent = nullptr);
    virtual ~MyInfoDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MYINFO_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnBack();
    afx_msg LRESULT OnMyInfoResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void RequestMyInfo();
    void FillFields(const std::string& body);
};
