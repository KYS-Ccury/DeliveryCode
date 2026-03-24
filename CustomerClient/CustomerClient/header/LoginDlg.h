#pragma once
#include "afxdialogex.h"
#include <atomic>
#include <string>

// ================================================================
//  LoginDlg.h  ─  로그인 + 서버 연결 상태 표시 (완전판)
//
//  [기능]
//  1. 서버 IP/Port 입력 + 연결 버튼 → 연결 상태 실시간 표시
//  2. 아이디/비밀번호 입력 → 로그인 (서버 연동)
//  3. 회원가입 버튼 → SignupDlg 열기
//  4. 서버 미연결 시 → 더미 로그인으로 개발 테스트 가능
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

    // ── 버튼 핸들러 ──────────────────────────────────────────
    afx_msg void    OnBnClickedOk();           // 로그인
    afx_msg void    OnBnClickedCancel();
    afx_msg void    OnBnClickedConnect();      // 서버 연결
    afx_msg void    OnBnClickedGotoSignup();   // 회원가입 창 열기

    // ── 서버 응답 메시지 핸들러 ──────────────────────────────
    afx_msg LRESULT OnLoginResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnConnectResult(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // ── 상태 ─────────────────────────────────────────────────
    std::atomic<bool> m_bWaiting{ false };   // 로그인 요청 중 중복 방지
    std::atomic<bool> m_bConnecting{ false };// 서버 연결 시도 중

    // ── 서버 응답 임시 저장 ───────────────────────────────────
    std::string m_strToken;
    std::string m_strServerUserID;

    // ── 내부 헬퍼 ────────────────────────────────────────────
    void UpdateConnStatusUI();               // 연결 상태 라벨 갱신
    void SetLoginError(const CString& msg);  // 에러 라벨 표시
    bool ReadLoginFields(CString& id, CString& pw); // 입력값 읽기
};
