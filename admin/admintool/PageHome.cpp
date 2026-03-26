/**
 * PageHome.cpp
 * ============================================================
 * 대시보드(홈) 페이지 구현부이다.
 * 서버에서 주문/라이더/리뷰 카운트를 가져와 카드 형태로 표시한다.
 *
 * ★ 서버 연동:
 *   CMD_ORDER_MONITOR (510) - 대기 주문 수
 *   CMD_RIDER_STATUS  (511) - 라이더 현황 수
 *   CMD_MANAGE_REVIEW (520) - 리뷰 수
 * ============================================================
 */

#include "pch.h"
#include "PageHome.h"
#include "admintool.h"      // GetSocket()
#include "PacketDef.h"      // CMD 상수

IMPLEMENT_DYNAMIC(PageHome, PageBase)

// ★ 헬퍼: App에서 소켓 가져오기
static CClientSocket& GetSocket()
{
    return ((CAdminToolApp*)AfxGetApp())->GetSocket();
}

// ============================================================
// 생성자 / 소멸자
// ============================================================
PageHome::PageHome(CWnd* pParent)
    : PageBase(IDD_PAGE_HOME, pParent)
    , m_nOrderCount(0)
    , m_nDispatchCount(0)
    , m_nReviewCount(0)
{
}

PageHome::~PageHome()
{
}

// ============================================================
// DDX
// ============================================================
void PageHome::DoDataExchange(CDataExchange* pDX)
{
    PageBase::DoDataExchange(pDX);
}

// ============================================================
// 초기화
// ============================================================
BOOL PageHome::OnInitDialog()
{
    PageBase::OnInitDialog();

    // 카드 제목용 폰트
    m_fontTitle.CreatePointFont(120, _T("맑은 고딕"));

    // 카드 숫자용 폰트
    m_fontCount.CreatePointFont(280, _T("맑은 고딕"));

    return TRUE;
}

// ============================================================
// 대시보드 카운트 설정
// ============================================================
void PageHome::SetCounts(int nOrder, int nDispatch, int nReview)
{
    m_nOrderCount = nOrder;
    m_nDispatchCount = nDispatch;
    m_nReviewCount = nReview;

    if (GetSafeHwnd())
        Invalidate(TRUE);
}

// ============================================================
// 카드 하나 그리기
// ============================================================
void PageHome::DrawCard(CDC* pDC, const CRect& rc, const CString& strTitle,
    int nCount, COLORREF clrAccent)
{
    // 카드 배경 (흰색 라운드 사각형)
    CBrush brBg(RGB(255, 255, 255));
    CPen penBorder(PS_SOLID, 1, RGB(220, 220, 220));
    CBrush* pOldBr = pDC->SelectObject(&brBg);
    CPen* pOldPn = pDC->SelectObject(&penBorder);
    pDC->RoundRect(rc, CPoint(8, 8));
    pDC->SelectObject(pOldBr);
    pDC->SelectObject(pOldPn);

    // 상단 액센트 바
    CRect rcAccent(rc.left + 1, rc.top + 1, rc.right - 1, rc.top + 5);
    pDC->FillSolidRect(rcAccent, clrAccent);

    // 제목
    CFont* pOldFont = pDC->SelectObject(&m_fontTitle);
    pDC->SetTextColor(RGB(100, 100, 100));
    CRect rcTitle(rc.left + 16, rc.top + 16, rc.right - 16, rc.top + 40);
    pDC->DrawText(strTitle, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    // 숫자
    pDC->SelectObject(&m_fontCount);
    pDC->SetTextColor(clrAccent);
    CString strCount;
    strCount.Format(_T("%d"), nCount);
    CRect rcCount(rc.left + 16, rc.top + 44, rc.right - 16, rc.bottom - 12);
    pDC->DrawText(strCount, &rcCount, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    pDC->SelectObject(pOldFont);
}

// ============================================================
// 배경 지우기 (깜빡임 방지)
// ============================================================
BOOL PageHome::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

// ============================================================
// 페인트: 대시보드 카드 3장 그리기
// ============================================================
void PageHome::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);

    // 더블 버퍼링
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    // 배경색
    memDC.FillSolidRect(rcClient, RGB(240, 242, 245));
    memDC.SetBkMode(TRANSPARENT);

    // 카드 레이아웃 계산
    const int CARD_MARGIN = 20;
    const int CARD_TOP = 30;
    const int CARD_H = 120;

    int nAvailW = rcClient.Width() - CARD_MARGIN * 4;
    int nCardW = nAvailW / 3;
    if (nCardW < 100) nCardW = 100;

    CRect rcCard1(CARD_MARGIN, CARD_TOP,
        CARD_MARGIN + nCardW, CARD_TOP + CARD_H);

    CRect rcCard2(CARD_MARGIN * 2 + nCardW, CARD_TOP,
        CARD_MARGIN * 2 + nCardW * 2, CARD_TOP + CARD_H);

    CRect rcCard3(CARD_MARGIN * 3 + nCardW * 2, CARD_TOP,
        CARD_MARGIN * 3 + nCardW * 3, CARD_TOP + CARD_H);

    DrawCard(&memDC, rcCard1, _T("대기 주문"), m_nOrderCount, RGB(52, 152, 219));
    DrawCard(&memDC, rcCard2, _T("라이더 현황"), m_nDispatchCount, RGB(46, 204, 113));
    DrawCard(&memDC, rcCard3, _T("리뷰"), m_nReviewCount, RGB(231, 76, 60));

    // 화면 전송
    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

// ============================================================
// 리사이즈
// ============================================================
void PageHome::OnSize(UINT nType, int cx, int cy)
{
    PageBase::OnSize(nType, cx, cy);
    if (GetSafeHwnd())
        Invalidate(TRUE);
}

// ============================================================
// ★ 서버에서 대시보드 데이터 로드
// ============================================================
void PageHome::LoadDataFromServer()
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    int nOrderCount = 0;
    int nDispatchCount = 0;
    int nReviewCount = 0;

    // --- 주문 모니터링 (510) ---
    {
        json reqBody;
        if (sock.SendAdminPacket(CMD_ORDER_MONITOR, reqBody))
        {
            RecvResult res = sock.RecvPacket();
            if (res.success && res.body.contains("orders")
                && res.body["orders"].is_array())
            {
                nOrderCount = static_cast<int>(res.body["orders"].size());
            }
        }
    }

    // --- 라이더 현황 (511) ---
    {
        json reqBody;
        if (sock.SendAdminPacket(CMD_RIDER_STATUS, reqBody))
        {
            RecvResult res = sock.RecvPacket();
            if (res.success && res.body.contains("riders")
                && res.body["riders"].is_array())
            {
                nDispatchCount = static_cast<int>(res.body["riders"].size());
            }
        }
    }

    // --- 리뷰 관리 (520) ---
    {
        json reqBody;
        reqBody["action"] = "list";
        if (sock.SendAdminPacket(CMD_MANAGE_REVIEW, reqBody))
        {
            RecvResult res = sock.RecvPacket();
            if (res.success && res.body.contains("reviews")
                && res.body["reviews"].is_array())
            {
                nReviewCount = static_cast<int>(res.body["reviews"].size());
            }
        }
    }

    // 카운트 반영
    SetCounts(nOrderCount, nDispatchCount, nReviewCount);
}

// ============================================================
// LoadData: 페이지 진입 시 호출
// ============================================================
void PageHome::LoadData()
{
    LoadDataFromServer();
    Invalidate(TRUE);
}

// ============================================================
// SaveData
// ============================================================
void PageHome::SaveData()
{
    AfxMessageBox(_T("Home Saved"));
}

// ============================================================
// 메시지 맵
// ============================================================
BEGIN_MESSAGE_MAP(PageHome, PageBase)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()