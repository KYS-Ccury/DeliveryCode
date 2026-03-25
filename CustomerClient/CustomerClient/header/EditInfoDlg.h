#pragma once
// ================================================================
//  EditInfoDlg.h  ─  내정보 수정 (비밀번호 변경) 다이얼로그
//
//  [연동 프로토콜]
//  ▶ 비밀번호 변경 요청
//    REQ: { "token":"...", "current_pw":"...", "new_pw":"..." }
//    RES: { "status":2000 }
//
//  resource.h 사용 IDC:
//    IDD_EDITINFO_DLG            157
//    IDC_EDIT_CUR_PW             2200
//    IDC_EDIT_NEW_PW             2201
//    IDC_EDIT_NEW_PW2            2202
//    IDC_STATIC_EDITINFO_ERR     2203
//    IDC_BTN_BACK                1400  (공통)
// ================================================================
#include "afxdialogex.h"

//#define IDD_EDITINFO_DLG            157

#define WM_EDITINFO_RESPONSE (WM_USER + 210)

class EditInfoDlg : public CDialogEx
{
    DECLARE_DYNAMIC(EditInfoDlg)
public:
    EditInfoDlg(CWnd* pParent = nullptr);
    virtual ~EditInfoDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_EDITINFO_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();      // 변경 완료
    afx_msg void    OnBnClickedBtnBack();
    afx_msg LRESULT OnEditInfoResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void SetErrorMsg(const CString& msg);
};
