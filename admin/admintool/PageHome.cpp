/**
 * PageHome.cpp
 * ============================================================
 * ★ 수정사항:
 *   1) OnLButtonDown → 카드/미니리스트 클릭 시 페이지 이동
 *   2) 미니 리스트(최근 주문/리뷰) 표시
 *   3) UTF-8 → CString 변환 (Utf8ToCString) 적용
 *   4) ★ 소켓 Lock/Unlock 추가 (폴링 스레드와 경합 방지)
 * ============================================================
 */

#include "pch.h"
#include "PageHome.h"
#include "admintool.h"
#include "PacketDef.h"
#include "PageInquiry.h"    // Utf8ToCString 헬퍼 사용

IMPLEMENT_DYNAMIC(PageHome, PageBase)

static CClientSocket& GetSocket()
{
    return ((CAdminToolApp*)AfxGetApp())->GetSocket();
}

PageHome::PageHome(CWnd* pParent)
    : PageBase(IDD_PAGE_HOME, pParent)
    , m_nOrderCount(0)
    , m_nDispatchCount(0)
    , m_nReviewCount(0)
{
    m_rcCardOrder.SetRectEmpty();
    m_rcCardDispatch.SetRectEmpty();
    m_rcCardReview.SetRectEmpty();
    m_rcMiniOrders.SetRectEmpty();
    m_rcMiniReviews.SetRectEmpty();
}

PageHome::~PageHome()
{
}

void PageHome::DoDataExchange(CDataExchange* pDX)
{
    PageBase::DoDataExchange(pDX);
}

BOOL PageHome::OnInitDialog()
{
    PageBase::OnInitDialog();
    m_fontTitle.CreatePointFont(120, _T("맑은 고딕"));
    m_fontCount.CreatePointFont(280, _T("맑은 고딕"));
    m_fontMini.CreatePointFont(90, _T("맑은 고딕"));
    m_fontMiniTitle.CreatePointFont(110, _T("맑은 고딕"));
    return TRUE;
}

void PageHome::SetCounts(int nOrder, int nDispatch, int nReview)
{
    m_nOrderCount = nOrder;
    m_nDispatchCount = nDispatch;
    m_nReviewCount = nReview;
    if (GetSafeHwnd())
        Invalidate(TRUE);
}

void PageHome::DrawCard(CDC* pDC, const CRect& rc, const CString& strTitle,
    int nCount, COLORREF clrAccent)
{
    CBrush brBg(RGB(255, 255, 255));
    CPen penBorder(PS_SOLID, 1, RGB(220, 220, 220));
    CBrush* pOldBr = pDC->SelectObject(&brBg);
    CPen* pOldPn = pDC->SelectObject(&penBorder);
    pDC->RoundRect(rc, CPoint(8, 8));
    pDC->SelectObject(pOldBr);
    pDC->SelectObject(pOldPn);

    CRect rcAccent(rc.left + 1, rc.top + 1, rc.right - 1, rc.top + 5);
    pDC->FillSolidRect(rcAccent, clrAccent);

    CFont* pOldFont = pDC->SelectObject(&m_fontTitle);
    pDC->SetTextColor(RGB(100, 100, 100));
    CRect rcTitle(rc.left + 16, rc.top + 16, rc.right - 16, rc.top + 40);
    pDC->DrawText(strTitle, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    pDC->SelectObject(&m_fontCount);
    pDC->SetTextColor(clrAccent);
    CString strCount;
    strCount.Format(_T("%d"), nCount);
    CRect rcCount(rc.left + 16, rc.top + 44, rc.right - 16, rc.bottom - 24);
    pDC->DrawText(strCount, &rcCount, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    pDC->SelectObject(&m_fontMini);
    pDC->SetTextColor(RGB(160, 160, 160));
    CRect rcHint(rc.left + 16, rc.bottom - 22, rc.right - 10, rc.bottom - 4);
    pDC->DrawText(_T("클릭하여 이동 >"), &rcHint, DT_RIGHT | DT_SINGLELINE);
    pDC->SelectObject(pOldFont);
}

void PageHome::DrawMiniList(CDC* pDC, const CRect& rc, const CString& strTitle,
    const std::vector<CString>& items)
{
    CBrush brBg(RGB(255, 255, 255));
    CPen penBorder(PS_SOLID, 1, RGB(220, 220, 220));
    CBrush* pOldBr = pDC->SelectObject(&brBg);
    CPen* pOldPn = pDC->SelectObject(&penBorder);
    pDC->RoundRect(rc, CPoint(8, 8));
    pDC->SelectObject(pOldBr);
    pDC->SelectObject(pOldPn);

    CFont* pOldFont = pDC->SelectObject(&m_fontMiniTitle);
    pDC->SetTextColor(RGB(50, 50, 50));
    CRect rcTitle(rc.left + 12, rc.top + 8, rc.right - 12, rc.top + 28);
    pDC->DrawText(strTitle, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    CPen penLine(PS_SOLID, 1, RGB(230, 230, 230));
    CPen* pOldPn2 = pDC->SelectObject(&penLine);
    pDC->MoveTo(rc.left + 12, rc.top + 30);
    pDC->LineTo(rc.right - 12, rc.top + 30);
    pDC->SelectObject(pOldPn2);

    pDC->SelectObject(&m_fontMini);
    pDC->SetTextColor(RGB(80, 80, 80));
    int nY = rc.top + 35;
    int nCount = (int)items.size();
    if (nCount > 5) nCount = 5;
    for (int i = 0; i < nCount; ++i)
    {
        CRect rcItem(rc.left + 16, nY, rc.right - 12, nY + 18);
        CString strLine;
        strLine.Format(_T("\x2022 %s"), (LPCTSTR)items[i]);
        pDC->DrawText(strLine, &rcItem, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
        nY += 20;
    }

    if (items.empty())
    {
        pDC->SetTextColor(RGB(180, 180, 180));
        CRect rcEmpty(rc.left + 16, nY, rc.right - 12, nY + 18);
        pDC->DrawText(_T("데이터 없음"), &rcEmpty, DT_LEFT | DT_SINGLELINE);
    }

    pDC->SetTextColor(RGB(52, 152, 219));
    CRect rcMore(rc.left + 12, rc.bottom - 22, rc.right - 12, rc.bottom - 6);
    pDC->DrawText(_T("더보기 >"), &rcMore, DT_RIGHT | DT_SINGLELINE);
    pDC->SelectObject(pOldFont);
}

BOOL PageHome::OnEraseBkgnd(CDC* pDC) { return TRUE; }

void PageHome::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);
    memDC.FillSolidRect(rcClient, RGB(240, 242, 245));
    memDC.SetBkMode(TRANSPARENT);

    const int CARD_MARGIN = 20;
    const int CARD_TOP = 30;
    const int CARD_H = 120;
    int nAvailW = rcClient.Width() - CARD_MARGIN * 4;
    int nCardW = nAvailW / 3;
    if (nCardW < 100) nCardW = 100;

    m_rcCardOrder.SetRect(CARD_MARGIN, CARD_TOP, CARD_MARGIN + nCardW, CARD_TOP + CARD_H);
    m_rcCardDispatch.SetRect(CARD_MARGIN * 2 + nCardW, CARD_TOP, CARD_MARGIN * 2 + nCardW * 2, CARD_TOP + CARD_H);
    m_rcCardReview.SetRect(CARD_MARGIN * 3 + nCardW * 2, CARD_TOP, CARD_MARGIN * 3 + nCardW * 3, CARD_TOP + CARD_H);

    DrawCard(&memDC, m_rcCardOrder, _T("대기 주문"), m_nOrderCount, RGB(52, 152, 219));
    DrawCard(&memDC, m_rcCardDispatch, _T("라이더 현황"), m_nDispatchCount, RGB(46, 204, 113));
    DrawCard(&memDC, m_rcCardReview, _T("리뷰"), m_nReviewCount, RGB(231, 76, 60));

    const int LIST_TOP = CARD_TOP + CARD_H + 20;
    int nListH = rcClient.Height() - LIST_TOP - 20;
    if (nListH < 60) nListH = 60;
    int nListW = (rcClient.Width() - CARD_MARGIN * 3) / 2;
    if (nListW < 100) nListW = 100;

    m_rcMiniOrders.SetRect(CARD_MARGIN, LIST_TOP, CARD_MARGIN + nListW, LIST_TOP + nListH);
    m_rcMiniReviews.SetRect(CARD_MARGIN * 2 + nListW, LIST_TOP, CARD_MARGIN * 2 + nListW * 2, LIST_TOP + nListH);

    DrawMiniList(&memDC, m_rcMiniOrders, _T("최근 주문"), m_recentOrders);
    DrawMiniList(&memDC, m_rcMiniReviews, _T("최근 리뷰"), m_recentReviews);

    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void PageHome::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_rcCardOrder.PtInRect(point) || m_rcMiniOrders.PtInRect(point))
    {
        if (m_fnGoDispatch)
            m_fnGoDispatch();
        return;
    }
    if (m_rcCardDispatch.PtInRect(point))
    {
        if (m_fnGoDispatch)
            m_fnGoDispatch();
        return;
    }
    if (m_rcCardReview.PtInRect(point) || m_rcMiniReviews.PtInRect(point))
    {
        if (m_fnGoReview)
            m_fnGoReview();
        return;
    }
    PageBase::OnLButtonDown(nFlags, point);
}

void PageHome::OnSize(UINT nType, int cx, int cy)
{
    PageBase::OnSize(nType, cx, cy);
    if (GetSafeHwnd()) Invalidate(TRUE);
}

// ============================================================
// ★ 서버에서 대시보드 데이터 로드 (Lock/Unlock 추가)
// ============================================================
void PageHome::LoadDataFromServer()
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    int nOrderCount = 0;
    int nDispatchCount = 0;
    int nReviewCount = 0;
    m_recentOrders.clear();
    m_recentReviews.clear();

    // --- 주문 모니터링 (510) ---
    {
        sock.Lock();  // ★

        json reqBody;
        if (sock.SendAdminPacket(CMD_ORDER_MONITOR, reqBody))
        {
            RecvResult res = sock.RecvPacket();

            sock.Unlock();  // ★

            if (res.success && res.body.contains("orders") && res.body["orders"].is_array())
            {
                auto& orders = res.body["orders"];
                nOrderCount = static_cast<int>(orders.size());
                int nMax = (nOrderCount > 5) ? 5 : nOrderCount;
                for (int i = 0; i < nMax; ++i)
                {
                    CString strId = Utf8ToCString(orders[i].value("order_id", ""));
                    CString strRest = Utf8ToCString(orders[i].value("restaurant_name", ""));
                    CString strStatus = Utf8ToCString(orders[i].value("status", ""));
                    CString strPrice = Utf8ToCString(orders[i].value("total_price", ""));
                    CString strItem;
                    strItem.Format(_T("#%s %s %s원 [%s]"),
                        (LPCTSTR)strId, (LPCTSTR)strRest, (LPCTSTR)strPrice, (LPCTSTR)strStatus);
                    m_recentOrders.push_back(strItem);
                }
            }
        }
        else
        {
            sock.Unlock();  // ★ Send 실패 시에도 Unlock
        }
    }

    // --- 라이더 현황 (511) ---
    {
        sock.Lock();  // ★

        json reqBody;
        if (sock.SendAdminPacket(CMD_RIDER_STATUS, reqBody))
        {
            RecvResult res = sock.RecvPacket();

            sock.Unlock();  // ★

            if (res.success && res.body.contains("riders") && res.body["riders"].is_array())
            {
                nDispatchCount = static_cast<int>(res.body["riders"].size());
            }
        }
        else
        {
            sock.Unlock();  // ★
        }
    }

    // --- 리뷰 관리 (520) ---
    {
        sock.Lock();  // ★

        json reqBody;
        reqBody["action"] = "list";
        if (sock.SendAdminPacket(CMD_MANAGE_REVIEW, reqBody))
        {
            RecvResult res = sock.RecvPacket();

            sock.Unlock();  // ★

            if (res.success && res.body.contains("reviews") && res.body["reviews"].is_array())
            {
                auto& reviews = res.body["reviews"];
                nReviewCount = static_cast<int>(reviews.size());
                int nMax = (nReviewCount > 5) ? 5 : nReviewCount;
                for (int i = 0; i < nMax; ++i)
                {
                    CString strUser = Utf8ToCString(reviews[i].value("customer_name", ""));
                    CString strContent = Utf8ToCString(reviews[i].value("content", ""));
                    if (strContent.GetLength() > 30)
                        strContent = strContent.Left(30) + _T("...");
                    CString strItem;
                    strItem.Format(_T("%s: %s"), (LPCTSTR)strUser, (LPCTSTR)strContent);
                    m_recentReviews.push_back(strItem);
                }
            }
        }
        else
        {
            sock.Unlock();  // ★
        }
    }

    SetCounts(nOrderCount, nDispatchCount, nReviewCount);
}

void PageHome::LoadData()
{
    LoadDataFromServer();
    Invalidate(TRUE);
}

void PageHome::SaveData()
{
    AfxMessageBox(_T("Home Saved"));
}

BEGIN_MESSAGE_MAP(PageHome, PageBase)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()