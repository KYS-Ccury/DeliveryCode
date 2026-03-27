/**
 * admintool.cpp
 * ============================================================
 * ★ 수정사항: 변경 없음 (기존 그대로)
 *   소켓은 CAdminToolApp 소멸자에서 자동 정리됨
 *   (CClientSocket 소멸자가 Disconnect + CleanupWinsock 호출)
 * ============================================================
 */

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

    // 로그인 다이얼로그 (서버 연결 + CMD_LOGIN 수행)
    CLoginDlg loginDlg;
    if (loginDlg.DoModal() != IDOK)
    {
        // 로그인 실패/취소 시 소켓 정리 후 종료
        m_socket.Disconnect();
        return FALSE;
    }

    // 로그인 성공 → 메인 다이얼로그
    CMainDialog dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    // 메인 종료 시 로그아웃 + 소켓 정리
    if (m_socket.IsConnected())
    {
        json logoutReq;
        m_socket.SendAdminPacket(CMD_LOGOUT, logoutReq);
        m_socket.Disconnect();
    }

    return FALSE;
}