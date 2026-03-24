// ================================================================
//  CustomerClientDlg.cpp  ─  앱 진입점 (로그아웃 후 재로그인 지원)
//
//  [흐름]
//  while(true)
//    LoginDlg → IDOK(로그인 성공) → MainHomeDlg
//    MainHomeDlg → IDCANCEL(로그아웃) → 다시 LoginDlg
//    LoginDlg → IDCANCEL → 앱 종료
// ================================================================
#include "pch.h"
#include "framework.h"
#include "CustomerClient.h"
#include "CustomerClientDlg.h"
#include "afxdialogex.h"
#include "LoginDlg.h"
#include "MainHomeDlg.h"
#include "AuthManager.h"
#include "NetworkManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};
CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
void CAboutDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx) END_MESSAGE_MAP()

// =================================================================

CCustomerClientDlg::CCustomerClientDlg(CWnd* pParent)
    : CDialogEx(IDD_CUSTOMERCLIENT_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCustomerClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCustomerClientDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

BOOL CCustomerClientDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu) {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // ── 이 창은 숨기고 LoginDlg부터 시작 ─────────────────────
    ShowWindow(SW_HIDE);

    // ── 로그인 → 메인 홈 → 로그아웃 → 재로그인 루프 ──────────
    while (true) {
        LoginDlg loginDlg;
        if (loginDlg.DoModal() != IDOK) {
            // 로그인 취소 또는 X 버튼 → 앱 종료
            NetworkManager::GetInstance().Disconnect();
            PostQuitMessage(0);
            return FALSE;
        }

        // 메인 홈 실행 (IDCANCEL = 로그아웃, IDOK = 앱 종료)
        MainHomeDlg mainDlg;
        INT_PTR nRet = mainDlg.DoModal();

        if (nRet != IDCANCEL) {
            // IDOK 또는 X 버튼 → 앱 종료
            NetworkManager::GetInstance().Disconnect();
            PostQuitMessage(0);
            return FALSE;
        }
        // IDCANCEL = 로그아웃 → 루프 계속 (LoginDlg 재표시)
        // AuthManager::Logout()은 MainHomeDlg에서 이미 호출됨
    }

    return FALSE;
}

void CCustomerClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CCustomerClientDlg::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND,
                    reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect; GetClientRect(&rect);
        dc.DrawIcon((rect.Width()  - cxIcon + 1) / 2,
                    (rect.Height() - cyIcon + 1) / 2, m_hIcon);
    } else {
        CDialogEx::OnPaint();
    }
}

HCURSOR CCustomerClientDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}
