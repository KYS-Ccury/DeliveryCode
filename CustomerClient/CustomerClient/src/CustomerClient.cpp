// CustomerClient.cpp : 앱 진입점
#include "pch.h"
#include "framework.h"
#include "CustomerClient.h"
#include "CustomerClientDlg.h"
#include "LoginDlg.h"
#include "MainHomeDlg.h"
#include "AuthManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CCustomerClientApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CCustomerClientApp::CCustomerClientApp()
{
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CCustomerClientApp theApp;

BOOL CCustomerClientApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC  = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    AfxEnableControlContainer();

    CShellManager* pShellManager = new CShellManager;
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
    SetRegistryKey(_T("BeMinCustomer"));

    // ── 로그인 다이얼로그 ──────────────────────────────
    {
        LoginDlg loginDlg;
        m_pMainWnd = &loginDlg;
        INT_PTR res = loginDlg.DoModal();
        // IDCANCEL(취소) 이면 종료
        if (res == IDCANCEL) {
            if (pShellManager) delete pShellManager;
            return FALSE;
        }
        // IDOK 이면 메인 홈으로 진입
    }

    // ── 메인 홈 화면 ───────────────────────────────────
    MainHomeDlg mainDlg;
    m_pMainWnd = &mainDlg;
    mainDlg.DoModal();

    if (pShellManager) delete pShellManager;

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
    ControlBarCleanUp();
#endif
    return FALSE;
}
