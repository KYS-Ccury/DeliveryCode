#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "admintool.h"
#include "MainDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 메인 다이얼로그 생성자이다.
CMainDialog::CMainDialog(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent)
    , m_pPageHome(nullptr)
    , m_pPageReview(nullptr)
    , m_pPageDispatch(nullptr)
    , m_pPageInquiry(nullptr)
    , m_pCurrentPage(nullptr)
{
    // 애플리케이션 아이콘을 로드한다.
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// 메인 다이얼로그 소멸자이다.
CMainDialog::~CMainDialog()
{
    // 각 페이지를 안전하게 제거한다.
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

// DDX 바인딩 함수이다.
void CMainDialog::DoDataExchange(CDataExchange* pDX)
{
    // 부모 클래스 DDX를 수행한다.
    CDialogEx::DoDataExchange(pDX);

    // 페이지 영역 static을 연결한다.
    DDX_Control(pDX, IDC_STATIC_PAGE, m_staticPage);

    // 제목 static을 연결한다.
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
END_MESSAGE_MAP()

// 메인 초기화 함수이다.
BOOL CMainDialog::OnInitDialog()
{
    // 부모 클래스 초기화를 수행한다.
    CDialogEx::OnInitDialog();

    // 아이콘을 설정한다.
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // 리사이즈/최대화 제거 (창 크기 고정)
    ModifyStyle(WS_THICKFRAME | WS_MAXIMIZEBOX, 0, SWP_FRAMECHANGED);

    // 자식 페이지 겹침 시 깜빡임을 줄이기 위한 스타일이다.
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // 제목 폰트를 만든다.
    m_font.CreatePointFont(120, _T("맑은 고딕"));
    m_staticTitle.SetFont(&m_font);

    // 중앙 페이지 영역도 자식들을 담기 좋게 스타일을 맞춘다.
    m_staticPage.ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // 페이지들을 생성한다.
    CreatePages();

    // 첫 화면은 홈 페이지로 진입한다.
    SwitchPage(m_pPageHome, false);

    return TRUE;
}

// 현재 중앙 페이지 영역의 클라이언트 좌표를 구한다.
CRect CMainDialog::GetPageAreaClientRect() const
{
    // 기본값으로 빈 사각형을 준비한다.
    CRect rcPage(0, 0, 0, 0);

    // 페이지 영역 컨트롤이 유효하지 않으면 빈 사각형을 반환한다.
    if (!::IsWindow(m_staticPage.GetSafeHwnd()))
        return rcPage;

    // static의 클라이언트 좌표를 구한다.
    m_staticPage.GetClientRect(&rcPage);

    return rcPage;
}

// 페이지들을 생성한다.
void CMainDialog::CreatePages()
{
    // 중앙 static 내부 클라이언트 기준 크기를 구한다.
    CRect rcPage = GetPageAreaClientRect();

    // 홈 페이지를 생성한다.
    m_pPageHome = new PageHome(&m_staticPage);
    m_pPageHome->Create(IDD_PAGE_HOME, &m_staticPage);
    m_pPageHome->SetCounts(5, 3, 3);
    m_pPageHome->m_fnGoInquiry = [this]()
        {
            // 홈에서 문의 리스트 클릭 시 문의 페이지로 이동한다.
            SwitchPage(m_pPageInquiry);
        };
    m_pPageHome->m_fnGoReview = [this]()
        {
            // 홈에서 차트 클릭 시 리뷰 페이지로 이동한다.
            SwitchPage(m_pPageReview);
        };
    m_pPageHome->m_fnGoDispatch = [this]()
        {
            // 홈에서 배차 리스트 클릭 시 배차 페이지로 이동한다.
            SwitchPage(m_pPageDispatch);
        };
    m_pPageHome->MoveWindow(rcPage);
    m_pPageHome->HidePage();

    // 리뷰 페이지를 생성한다.
    m_pPageReview = new PageReview(&m_staticPage);
    m_pPageReview->Create(IDD_PAGE_REVIEW, &m_staticPage);
    m_pPageReview->MoveWindow(rcPage);
    m_pPageReview->HidePage();

    // 배차 페이지를 생성한다.
    m_pPageDispatch = new PageDispatch(&m_staticPage);
    m_pPageDispatch->Create(IDD_PAGE_DISPATCH, &m_staticPage);
    m_pPageDispatch->MoveWindow(rcPage);
    m_pPageDispatch->HidePage();

    // 문의 페이지를 생성한다.
    m_pPageInquiry = new PageInquiry(&m_staticPage);
    m_pPageInquiry->Create(IDD_PAGE_INQUIRY, &m_staticPage);
    m_pPageInquiry->MoveWindow(rcPage);
    m_pPageInquiry->HidePage();
}

// 특정 페이지를 중앙 영역 크기에 맞춘다.
void CMainDialog::ResizePageToArea(PageBase* pPage)
{
    // 페이지나 static이 유효하지 않으면 종료한다.
    if (!pPage || !::IsWindow(m_staticPage.GetSafeHwnd()))
        return;

    // static 내부 클라이언트 좌표를 가져온다.
    CRect rcPage = GetPageAreaClientRect();

    // 페이지를 부모 static 내부 (0,0) 기준으로 맞춘다.
    pPage->MoveWindow(rcPage);
}

// 페이지 전환 함수이다.
void CMainDialog::SwitchPage(PageBase* pNewPage, bool bSaveHistory)
{
    // 대상 페이지가 없으면 종료한다.
    if (!pNewPage)
        return;

    // 같은 페이지로 다시 전환하려는 경우에는 크기만 보정하고 데이터만 새로 그린다.
    if (pNewPage == m_pCurrentPage)
    {
        ResizePageToArea(pNewPage);
        pNewPage->LoadData();
        pNewPage->Invalidate(TRUE);
        pNewPage->UpdateWindow();
        UpdateTitle(pNewPage->GetPageName());
        return;
    }

    // 현재 페이지가 있고 히스토리를 쌓아야 하면 저장한다.
    if (bSaveHistory && m_pCurrentPage)
        m_pageHistory.Add(m_pCurrentPage);

    // 기존 페이지를 숨긴다.
    if (m_pCurrentPage)
        m_pCurrentPage->HidePage();

    // 현재 페이지를 갱신한다.
    m_pCurrentPage = pNewPage;

    // 중앙 영역 크기에 맞게 페이지 크기를 조절한다.
    ResizePageToArea(m_pCurrentPage);

    // 페이지 데이터를 갱신한다.
    m_pCurrentPage->LoadData();

    // 페이지를 표시한다.
    m_pCurrentPage->ShowPage();

    // 제목을 갱신한다.
    UpdateTitle(m_pCurrentPage->GetPageName());
}

// 제목을 갱신한다.
void CMainDialog::UpdateTitle(const CString& strTitle)
{
    // 제목 static이 유효하면 문자열을 반영한다.
    if (::IsWindow(m_staticTitle.GetSafeHwnd()))
        m_staticTitle.SetWindowText(strTitle);
}

// 리뷰 메뉴 버튼 처리이다.
void CMainDialog::OnBtnMenuReview()
{
    // 리뷰 페이지로 이동한다.
    SwitchPage(m_pPageReview);
}

// 배차 메뉴 버튼 처리이다.
void CMainDialog::OnBtnMenuDispatch()
{
    // 배차 페이지로 이동한다.
    SwitchPage(m_pPageDispatch);
}

// 문의 메뉴 버튼 처리이다.
void CMainDialog::OnBtnMenuInquiry()
{
    // 문의 페이지로 이동한다.
    SwitchPage(m_pPageInquiry);
}

// 홈 버튼 처리이다.
void CMainDialog::OnBtnHome()
{
    // 홈 이동은 새 시작이므로 뒤로가기 히스토리를 비운다.
    m_pageHistory.RemoveAll();

    // 홈 페이지로 이동한다.
    SwitchPage(m_pPageHome, false);
}

// 뒤로가기 버튼 처리이다.
void CMainDialog::OnBtnBack()
{
    // 히스토리가 비어 있으면 홈으로 보낸다.
    if (m_pageHistory.GetSize() <= 0)
    {
        SwitchPage(m_pPageHome, false);
        return;
    }

    // 마지막 페이지를 꺼낸다.
    PageBase* pPrev = m_pageHistory[m_pageHistory.GetUpperBound()];
    m_pageHistory.RemoveAt(m_pageHistory.GetUpperBound());

    // 이전 페이지로 이동한다.
    SwitchPage(pPrev, false);
}

// 저장 버튼 처리이다.
void CMainDialog::OnBtnSave()
{
    // 현재 페이지가 있으면 페이지별 저장 동작을 호출한다.
    if (m_pCurrentPage)
    {
        m_pCurrentPage->SaveData();
    }
    else
    {
        AfxMessageBox(_T("No page to save"));
    }
}

// 닫기 처리이다.
void CMainDialog::OnClose()
{
    // 다이얼로그를 종료한다.
    EndDialog(IDOK);
}

// 페인트 처리이다.
void CMainDialog::OnPaint()
{
    // 최소화 상태이면 아이콘을 직접 그린다.
    if (IsIconic())
    {
        CPaintDC dc(this);

        // 아이콘 지우기 메시지를 보낸다.
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 아이콘 크기를 구한다.
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);

        // 클라이언트 영역을 구한다.
        CRect rect;
        GetClientRect(&rect);

        // 중앙 좌표를 계산한다.
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 아이콘을 그린다.
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        // 일반 상태에서는 기본 페인트를 사용한다.
        CDialogEx::OnPaint();
    }
}

// 드래그 아이콘 반환 함수이다.
HCURSOR CMainDialog::OnQueryDragIcon()
{
    // 현재 아이콘 핸들을 커서로 반환한다.
    return static_cast<HCURSOR>(m_hIcon);
}