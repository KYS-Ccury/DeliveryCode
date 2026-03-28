/**
 * MainDialog.cpp
 * ============================================================
 * ★ 수정사항:
 *   1) OnInitDialog에서 폴링 시작 (StartPolling)
 *   2) OnClose에서 폴링 중지 (StopPolling)
 *   3) WM_POLL_xxx 메시지 핸들러 추가
 *   4) 기존 페이지 전환/레이아웃 로직 100% 유지
 * ============================================================
 */

#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "admintool.h"
#include "MainDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMainDialog::CMainDialog(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent)
    , m_pPageHome(nullptr)
    , m_pPageReview(nullptr)
    , m_pPageDispatch(nullptr)
    , m_pPageInquiry(nullptr)
    , m_pCurrentPage(nullptr)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMainDialog::~CMainDialog()
{
    if (m_pPageHome)
    {
        if (::IsWindow(m_pPageHome->GetSafeHwnd()))
            m_pPageHome->DestroyWindow();
        delete m_pPageHome;
        m_pPageHome = nullptr;
    }

    if (m_pPageReview)
    {
        if (::IsWindow(m_pPageReview->GetSafeHwnd()))
            m_pPageReview->DestroyWindow();
        delete m_pPageReview;
        m_pPageReview = nullptr;
    }

    if (m_pPageDispatch)
    {
        if (::IsWindow(m_pPageDispatch->GetSafeHwnd()))
            m_pPageDispatch->DestroyWindow();
        delete m_pPageDispatch;
        m_pPageDispatch = nullptr;
    }

    if (m_pPageInquiry)
    {
        if (::IsWindow(m_pPageInquiry->GetSafeHwnd()))
            m_pPageInquiry->DestroyWindow();
        delete m_pPageInquiry;
        m_pPageInquiry = nullptr;
    }
}

void CMainDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_PAGE, m_staticPage);
    DDX_Control(pDX, IDC_STATIC_TITLE, m_staticTitle);
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_MENU_REVIEW, &CMainDialog::OnBtnMenuReview)
    ON_BN_CLICKED(IDC_BTN_MENU_DISPATCH, &CMainDialog::OnBtnMenuDispatch)
    ON_BN_CLICKED(IDC_BTN_MENU_INQUIRY, &CMainDialog::OnBtnMenuInquiry)
    ON_BN_CLICKED(IDC_BTN_HOME, &CMainDialog::OnBtnHome)
    ON_BN_CLICKED(IDC_BTN_BACK, &CMainDialog::OnBtnBack)
    ON_BN_CLICKED(IDC_BTN_SAVE, &CMainDialog::OnBtnSave)
    // ★ 폴링 커스텀 메시지 핸들러
    ON_MESSAGE(WM_POLL_HEARTBEAT_OK, &CMainDialog::OnPollHeartbeatOk)
    ON_MESSAGE(WM_POLL_HEARTBEAT_FAIL, &CMainDialog::OnPollHeartbeatFail)
    ON_MESSAGE(WM_POLL_NEW_MESSAGES, &CMainDialog::OnPollNewMessages)
END_MESSAGE_MAP()

BOOL CMainDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    ModifyStyle(WS_THICKFRAME | WS_MAXIMIZEBOX, 0, SWP_FRAMECHANGED);
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    m_font.CreatePointFont(120, _T("맑은 고딕"));
    m_staticTitle.SetFont(&m_font);

    m_staticPage.ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    CreatePages();

    SwitchPage(m_pPageHome, false);

    // ★ 폴링 시작 (5초 주기, 이 다이얼로그에 PostMessage)
    CAdminToolApp* pApp = (CAdminToolApp*)AfxGetApp();
    CClientSocket& sock = pApp->GetSocket();
    if (sock.IsConnected())
    {
        sock.StartPolling(GetSafeHwnd(), 5000);
    }

    return TRUE;
}

CRect CMainDialog::GetPageAreaClientRect() const
{
    CRect rcPage(0, 0, 0, 0);
    if (!::IsWindow(m_staticPage.GetSafeHwnd()))
        return rcPage;
    m_staticPage.GetClientRect(&rcPage);
    return rcPage;
}

void CMainDialog::CreatePages()
{
    CRect rcPage = GetPageAreaClientRect();

    m_pPageHome = new PageHome(&m_staticPage);
    m_pPageHome->Create(IDD_PAGE_HOME, &m_staticPage);
    m_pPageHome->SetCounts(5, 3, 3);
    m_pPageHome->m_fnGoInquiry = [this]()
        {
            SwitchPage(m_pPageInquiry);
        };
    m_pPageHome->m_fnGoReview = [this]()
        {
            SwitchPage(m_pPageReview);
        };
    m_pPageHome->m_fnGoDispatch = [this]()
        {
            SwitchPage(m_pPageDispatch);
        };
    m_pPageHome->MoveWindow(rcPage);
    m_pPageHome->HidePage();

    m_pPageReview = new PageReview(&m_staticPage);
    m_pPageReview->Create(IDD_PAGE_REVIEW, &m_staticPage);
    m_pPageReview->MoveWindow(rcPage);
    m_pPageReview->HidePage();

    m_pPageDispatch = new PageDispatch(&m_staticPage);
    m_pPageDispatch->Create(IDD_PAGE_DISPATCH, &m_staticPage);
    m_pPageDispatch->MoveWindow(rcPage);
    m_pPageDispatch->HidePage();

    m_pPageInquiry = new PageInquiry(&m_staticPage);
    m_pPageInquiry->Create(IDD_PAGE_INQUIRY, &m_staticPage);
    m_pPageInquiry->MoveWindow(rcPage);
    m_pPageInquiry->HidePage();
}

void CMainDialog::ResizePageToArea(PageBase* pPage)
{
    if (!pPage || !::IsWindow(m_staticPage.GetSafeHwnd()))
        return;
    CRect rcPage = GetPageAreaClientRect();
    pPage->MoveWindow(rcPage);
}

void CMainDialog::SwitchPage(PageBase* pNewPage, bool bSaveHistory)
{
    if (!pNewPage)
        return;

    if (pNewPage == m_pCurrentPage)
    {
        ResizePageToArea(pNewPage);
        pNewPage->LoadData();
        pNewPage->Invalidate(TRUE);
        pNewPage->UpdateWindow();
        UpdateTitle(pNewPage->GetPageName());
        return;
    }

    if (bSaveHistory && m_pCurrentPage)
        m_pageHistory.Add(m_pCurrentPage);

    if (m_pCurrentPage)
        m_pCurrentPage->HidePage();

    m_pCurrentPage = pNewPage;

    ResizePageToArea(m_pCurrentPage);

    m_pCurrentPage->LoadData();

    m_pCurrentPage->ShowPage();

    UpdateTitle(m_pCurrentPage->GetPageName());

    // ★ 페이지 전환 시 폴링 채팅방 ID 갱신
    CAdminToolApp* pApp = (CAdminToolApp*)AfxGetApp();
    CClientSocket& sock = pApp->GetSocket();

    if (m_pCurrentPage == m_pPageInquiry)
    {
        // 문의 페이지 진입 → 현재 선택된 방 ID가 있으면 설정
        // (PageInquiry가 LoadData 시 방 선택하면 거기서 설정됨)
        // 여기서는 빈 값으로 초기화, 방 선택 시 PageInquiry가 설정
        sock.SetPollingRoomId("");
    }
    else
    {
        // 다른 페이지로 이동 → 채팅 폴링 중지
        sock.SetPollingRoomId("");
    }
}

void CMainDialog::UpdateTitle(const CString& strTitle)
{
    if (::IsWindow(m_staticTitle.GetSafeHwnd()))
        m_staticTitle.SetWindowText(strTitle);
}

void CMainDialog::OnBtnMenuReview()
{
    SwitchPage(m_pPageReview);
}

void CMainDialog::OnBtnMenuDispatch()
{
    SwitchPage(m_pPageDispatch);
}

void CMainDialog::OnBtnMenuInquiry()
{
    SwitchPage(m_pPageInquiry);
}

void CMainDialog::OnBtnHome()
{
    m_pageHistory.RemoveAll();
    SwitchPage(m_pPageHome, false);
}

void CMainDialog::OnBtnBack()
{
    if (m_pageHistory.GetSize() <= 0)
    {
        SwitchPage(m_pPageHome, false);
        return;
    }

    PageBase* pPrev = m_pageHistory[m_pageHistory.GetUpperBound()];
    m_pageHistory.RemoveAt(m_pageHistory.GetUpperBound());
    SwitchPage(pPrev, false);
}

void CMainDialog::OnBtnSave()
{
    if (m_pCurrentPage)
    {
        m_pCurrentPage->SaveData();
    }
    else
    {
        AfxMessageBox(_T("No page to save"));
    }
}

// ============================================================
// ★ 닫기 → 폴링 중지 후 종료
// ============================================================
void CMainDialog::OnClose()
{
    CAdminToolApp* pApp = (CAdminToolApp*)AfxGetApp();
    CClientSocket& sock = pApp->GetSocket();
    sock.StopPolling();

    EndDialog(IDOK);
}

void CMainDialog::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CMainDialog::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// ============================================================
// ★ 폴링 결과 핸들러
// ============================================================

LRESULT CMainDialog::OnPollHeartbeatOk(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // 서버 연결 정상 — 특별한 동작 불필요
    // 필요하면 상태바에 "연결됨" 표시 가능
    return 0;
}

LRESULT CMainDialog::OnPollHeartbeatFail(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // 서버 연결 끊김 알림
    AfxMessageBox(_T("서버와의 연결이 끊어졌습니다.\n다시 로그인해주세요."));
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CMainDialog::OnPollNewMessages(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // 새 채팅 메시지 도착 → 현재 문의 페이지가 열려있으면 새로고침
    if (m_pCurrentPage == m_pPageInquiry && m_pPageInquiry)
    {
        m_pPageInquiry->OnPollRefreshChat();
    }
    return 0;
}