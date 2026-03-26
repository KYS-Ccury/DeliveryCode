#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "LoginDlg.h"

CLoginDlg::CLoginDlg(CWnd* pParent)
    : CDialogEx(IDD_LOGIN_DLG, pParent)
{
}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_ID, m_editId);
    DDX_Control(pDX, IDC_EDIT_PW, m_editPw);
}

BOOL CLoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    return TRUE;
}

void CLoginDlg::OnBtnLogin()
{
    CString strId, strPw;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);

    if (strId == _T("admin") && strPw == _T("1234"))
    {
        EndDialog(IDOK);
    }
    else
    {
        AfxMessageBox(_T("Invalid ID or Password"));
    }
}

void CLoginDlg::OnOK()
{
    OnBtnLogin();
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_LOGIN, &CLoginDlg::OnBtnLogin)
END_MESSAGE_MAP()