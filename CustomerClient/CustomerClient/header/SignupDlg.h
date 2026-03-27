#pragma once
#include "afxdialogex.h"
#include <atomic>
#include <string>

// ================================================================
//  SignupDlg.h  ─  회원가입 화면
//
//  [기능]
//  1. 아이디/비밀번호/비번확인/이름/전화번호/주소/역할 입력
//  2. 클라이언트 측 유효성 검사
//  3. 서버 전송 (REQ_SIGNUP 100)
//  4. 성공 시 m_strResultID 에 아이디 저장 → LoginDlg 자동 입력
//
//  [IDC] ← resource.h 및 RC 에 추가 필요
//    IDD_SIGNUP_DLG           1950
//    IDC_EDIT_SIGNUP_ID       1951
//    IDC_EDIT_SIGNUP_PW       1952
//    IDC_EDIT_SIGNUP_PW2      1953
//    IDC_EDIT_SIGNUP_NAME     1954
//    IDC_EDIT_SIGNUP_PHONE    1955
//    IDC_EDIT_SIGNUP_ADDR     1956
//    IDC_COMBO_SIGNUP_ROLE    1957
//    IDC_STATIC_SIGNUP_ERR    1958
// ================================================================
class SignupDlg : public CDialogEx
{
    DECLARE_DYNAMIC(SignupDlg)
public:
    SignupDlg(CWnd* pParent = nullptr);
    virtual ~SignupDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SIGNUP_DLG };
#endif

    // 회원가입 성공 후 LoginDlg에 전달할 아이디
    CString m_strResultID;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedCancel();

    // 서버 응답 핸들러
    afx_msg LRESULT OnSignupResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    std::atomic<bool> m_bWaiting{ false };

    void SetError(const CString& msg);
    bool ValidateInputs(CString& outID, CString& outPW,
                        CString& outName, CString& outPhone,
                        CString& outAddr, int& outRole);
};
