#include "pch.h"
#include "framework.h"
#include "admintool.h"
#include "LoginDlg.h"
#include "MainDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CAdminToolApp, CWinAppEx)
END_MESSAGE_MAP()

CAdminToolApp theApp;

CAdminToolApp::CAdminToolApp()
{
}

BOOL CAdminToolApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls = {};
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinAppEx::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(_T("AdminToolApp"));

    CLoginDlg loginDlg;
    if (loginDlg.DoModal() != IDOK)
        return FALSE;

    CMainDialog dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}