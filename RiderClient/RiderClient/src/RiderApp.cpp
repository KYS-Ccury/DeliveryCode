#include "pch.h"
#include "RiderApp.h"
#include "LoginDlg.h"

RiderApp theApp;
BEGIN_MESSAGE_MAP(RiderApp, CWinApp)
END_MESSAGE_MAP()

RiderApp::RiderApp() {}

BOOL RiderApp::InitInstance()
{
    CWinApp::InitInstance();
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);
    LoginDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
    return FALSE;
}

int RiderApp::ExitInstance()
{
    AppContext::Get().socket.Disconnect();
    return CWinApp::ExitInstance();
}
