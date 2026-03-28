
#include "pch.h"
#include "framework.h"
#include "Owner.h"
#include "OwnerDlg.h"
#include "CAuthDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COwnerApp

BEGIN_MESSAGE_MAP(COwnerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// COwnerApp 생성

COwnerApp::COwnerApp()
{
	
}


COwnerApp theApp;


// COwnerApp 초기화

BOOL COwnerApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

    CShellManager* pShellManager = new CShellManager;
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // =========================================================
    // 🚨 [적용된 핵심 로직] 통신 포인터 없이 깔끔하게 처리
    // =========================================================
    CLoginDlg loginDlg;

    // 로그인 창에서 버튼을 눌렀을 때 내부적으로 CNetClient를 사용함
    if (loginDlg.DoModal() == IDOK)
    {
        // 로그인 성공 시 메인 다이얼로그 실행
        COwnerDlg dlg;
        m_pMainWnd = &dlg; // 메인 윈도우 교체
        dlg.DoModal();
    }
    // =========================================================

    if (pShellManager != nullptr) delete pShellManager;

    return FALSE; // 다이얼로그 기반 앱은 종료 시 FALSE 리턴
}