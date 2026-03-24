#pragma once
#include "afxdialogex.h"
#include <string>
#include <atomic>

// ================================================================
//  LoginDlg.h  ─  로그인 화면 (수정본)
//  IDC_EDIT_ID, IDC_EDIT_PW, IDC_BTN_SIGNUP 는 resource.h 에 없음
//  → DDX 바인딩 제거, GetDlgItemText 방식으로 처리
// ================================================================
class LoginDlg : public CDialogEx
{
    DECLARE_DYNAMIC(LoginDlg)
public:
    LoginDlg(CWnd* pParent = nullptr);
    virtual ~LoginDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_LOGIN_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedCancel();
    afx_msg LRESULT OnLoginResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    std::atomic<bool> m_bWaiting;   // 중복 요청 방지
    std::string       m_strToken;   // 서버 발급 토큰
    std::string       m_strUserID;  // 서버 확인된 login_id
};
