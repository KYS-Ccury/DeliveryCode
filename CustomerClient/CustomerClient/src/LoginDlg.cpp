#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "LoginDlg.h"
#include "AuthManager.h"

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

LoginDlg::LoginDlg(CWnd* pParent)
    : CDialogEx(IDD_LOGIN_DLG, pParent) {}
LoginDlg::~LoginDlg() {}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,     &LoginDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &LoginDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

BOOL LoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    return TRUE;
}

void LoginDlg::OnBnClickedOk()
{
    // 서버 연동 전 임시 처리: 바로 메인 화면 진입
    AuthManager::GetInstance().Login("test_user", "test1234");
    CDialogEx::OnOK();
}

void LoginDlg::OnBnClickedCancel()
{
    CDialogEx::OnCancel();
}
