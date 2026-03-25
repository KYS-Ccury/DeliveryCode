#include "pch.h"
#include "PageHome.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

IMPLEMENT_DYNAMIC(PageHome, PageBase)

// 홈 페이지 생성자이다.
PageHome::PageHome(CWnd* pParent)
    : PageBase(IDD_PAGE_HOME, pParent)
    , m_nReviewCount(5)
    , m_nDispatchCount(3)
    , m_nInquiryCount(3)
    , m_nHoverSlice(-1)
{
    // 슬라이스 배열을 기본값으로 초기화한다.
    for (int i = 0; i < 5; ++i)
    {
        m_slices[i].startAngle = 0.0;
        m_slices[i].endAngle = 0.0;
        m_slices[i].color = RGB(200, 200, 200);
        m_slices[i].count = 0;
    }
}

// 홈 페이지 소멸자이다.
PageHome::~PageHome()
{
    // 이미지리스트가 살아 있으면 제거한다.
    if (m_imgDispatch.GetSafeHandle())
        m_imgDispatch.DeleteImageList();

    if (m_imgInquiry.GetSafeHandle())
        m_imgInquiry.DeleteImageList();
}

// DDX 함수이다.
void PageHome::DoDataExchange(CDataExchange* pDX)
{
    // 부모 클래스 DDX를 수행한다.
    PageBase::DoDataExchange(pDX);
}

// 각도를 0 ~ 2π 범위로 정규화한다.
double PageHome::NormalizeAngle(double dAngle) const
{
    // 음수/초과 각도를 0 ~ 2π 안으로 맞춘다.
    while (dAngle < 0.0)
        dAngle += (2.0 * M_PI);

    while (dAngle >= (2.0 * M_PI))
        dAngle -= (2.0 * M_PI);

    return dAngle;
}

// 슬라이스 데이터를 구성한다.
void PageHome::BuildSlices()
{
    // 더미 평점 분포 데이터이다.
    // 5점, 4점, 3점, 2점, 1점 순서이다.
    int arrData[5] = { 3, 5, 7, 2, 1 };

    // 각 조각의 색상이다.
    COLORREF arrColor[5] =
    {
        RGB(255, 215,   0),
        RGB(255, 165,   0),
        RGB(93, 173, 226),
        RGB(175, 122, 197),
        RGB(231,  76,  60)
    };

    // 전체 합계를 구한다.
    int nTotal = 0;
    for (int i = 0; i < 5; ++i)
        nTotal += arrData[i];

    // 시작 각도는 12시 방향으로 맞춘다.
    double dAngle = -M_PI / 2.0;

    // 각 슬라이스의 시작/끝 각도를 계산한다.
    for (int i = 0; i < 5; ++i)
    {
        // 비율에 따른 각도 길이이다.
        double dSpan = (nTotal > 0)
            ? (2.0 * M_PI * static_cast<double>(arrData[i]) / static_cast<double>(nTotal))
            : 0.0;

        // 그리기용 실제 각도를 저장한다.
        m_slices[i].startAngle = dAngle;
        m_slices[i].endAngle = dAngle + dSpan;
        m_slices[i].color = arrColor[i];
        m_slices[i].count = arrData[i];

        // 다음 시작 각도를 갱신한다.
        dAngle += dSpan;
    }
}

// 초기화 함수이다.
BOOL PageHome::OnInitDialog()
{
    // 부모 클래스 초기화를 수행한다.
    PageBase::OnInitDialog();

    // 공통 폰트를 만든다.
    m_font.CreatePointFont(110, _T("맑은 고딕"));

    // 배경색을 시스템 흰색 계열로 맞춘다.
    SetBackgroundColor(RGB(255, 255, 255));

    // 현재 크기를 기준으로 차트/리스트 위치를 계산한다.
    UpdateLayout();

    // 배차 리스트를 생성한다.
    m_listDispatch.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
        CRect(0, 0, 0, 0),
        this,
        DISPATCH_ID
    );

    // 문의 리스트를 생성한다.
    m_listInquiry.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
        CRect(0, 0, 0, 0),
        this,
        INQUIRY_ID
    );

    // 폰트를 적용한다.
    m_listDispatch.SetFont(&m_font);
    m_listInquiry.SetFont(&m_font);

    // 리스트를 초기화한다.
    InitDispatchList();
    InitInquiryList();

    // 더미 데이터를 넣는다.
    InsertDispatchDummy();
    InsertInquiryDummy();

    // 차트 데이터를 구성한다.
    BuildSlices();

    // 현재 크기에 맞게 실제 위치를 다시 맞춘다.
    UpdateLayout();

    return TRUE;
}

// 배차 리스트 초기화이다.
void PageHome::InitDispatchList()
{
    // 더미 이미지리스트를 연결한다.
    m_imgDispatch.Create(1, 24, ILC_COLOR, 0, 1);
    m_listDispatch.SetImageList(&m_imgDispatch, LVSIL_SMALL);

    // 그리드와 전체행 선택을 켠다.
    m_listDispatch.SetExtendedStyle(
        m_listDispatch.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT
    );

    // 컬럼을 추가한다.
    m_listDispatch.InsertColumn(0, _T("OrderID"), LVCFMT_LEFT, 100);
    m_listDispatch.InsertColumn(1, _T("Rider"), LVCFMT_LEFT, 80);
    m_listDispatch.InsertColumn(2, _T("Status"), LVCFMT_CENTER, 70);
}

// 문의 리스트 초기화이다.
void PageHome::InitInquiryList()
{
    // 더미 이미지리스트를 연결한다.
    m_imgInquiry.Create(1, 24, ILC_COLOR, 0, 1);
    m_listInquiry.SetImageList(&m_imgInquiry, LVSIL_SMALL);

    // 그리드와 전체행 선택을 켠다.
    m_listInquiry.SetExtendedStyle(
        m_listInquiry.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT
    );

    // 컬럼을 추가한다.
    m_listInquiry.InsertColumn(0, _T("User"), LVCFMT_LEFT, 100);
    m_listInquiry.InsertColumn(1, _T("Last Message"), LVCFMT_LEFT, 300);
}

// 배차 더미 데이터 삽입이다.
void PageHome::InsertDispatchDummy()
{
    // 기존 항목을 지운다.
    m_listDispatch.DeleteAllItems();

    // 더미 행을 추가한다.
    int nRow = 0;

    nRow = m_listDispatch.InsertItem(0, _T("ORD-001"));
    m_listDispatch.SetItemText(nRow, 1, _T("Kim"));
    m_listDispatch.SetItemText(nRow, 2, _T("배송중"));

    nRow = m_listDispatch.InsertItem(1, _T("ORD-002"));
    m_listDispatch.SetItemText(nRow, 1, _T("Lee"));
    m_listDispatch.SetItemText(nRow, 2, _T("배차완료"));

    nRow = m_listDispatch.InsertItem(2, _T("ORD-003"));
    m_listDispatch.SetItemText(nRow, 1, _T("Park"));
    m_listDispatch.SetItemText(nRow, 2, _T("대기중"));

    nRow = m_listDispatch.InsertItem(3, _T("ORD-004"));
    m_listDispatch.SetItemText(nRow, 1, _T("Choi"));
    m_listDispatch.SetItemText(nRow, 2, _T("완료"));

    nRow = m_listDispatch.InsertItem(4, _T("ORD-005"));
    m_listDispatch.SetItemText(nRow, 1, _T("Jung"));
    m_listDispatch.SetItemText(nRow, 2, _T("대기중"));
}

// 문의 더미 데이터 삽입이다.
void PageHome::InsertInquiryDummy()
{
    // 기존 항목을 지운다.
    m_listInquiry.DeleteAllItems();

    // 더미 행을 추가한다.
    int nRow = 0;

    nRow = m_listInquiry.InsertItem(0, _T("user001"));
    m_listInquiry.SetItemText(nRow, 1, _T("Order not arrived yet"));

    nRow = m_listInquiry.InsertItem(1, _T("user002"));
    m_listInquiry.SetItemText(nRow, 1, _T("I want a refund"));

    nRow = m_listInquiry.InsertItem(2, _T("user003"));
    m_listInquiry.SetItemText(nRow, 1, _T("Is my order confirmed?"));
}

// 외부 카운트 설정 함수이다.
void PageHome::SetCounts(int nReview, int nDispatch, int nInquiry)
{
    // 전달받은 값을 멤버에 저장한다.
    m_nReviewCount = nReview;
    m_nDispatchCount = nDispatch;
    m_nInquiryCount = nInquiry;
}

// 홈 데이터 갱신 함수이다.
void PageHome::LoadData()
{
    // 현재 레이아웃을 다시 맞춘다.
    UpdateLayout();

    // 리스트를 다시 채운다.
    InsertDispatchDummy();
    InsertInquiryDummy();

    // 차트 데이터를 다시 계산한다.
    BuildSlices();

    // hover 상태를 초기화한다.
    m_nHoverSlice = -1;

    // 다시 그리게 한다.
    Invalidate(FALSE);
    UpdateWindow();
}

// 현재 크기에 맞게 레이아웃을 계산한다.
void PageHome::UpdateLayout()
{
    // 클라이언트 영역을 구한다.
    CRect rcClient;
    GetClientRect(&rcClient);

    // 너무 작으면 계산하지 않는다.
    if (rcClient.Width() <= 0 || rcClient.Height() <= 0)
        return;

    // 전체 여백이다.
    const int nOuterMargin = 8;

    // 상단/하단 간 간격이다.
    const int nVerticalGap = 8;

    // 좌측 차트와 우측 배차 리스트 간 간격이다.
    const int nHorizontalGap = 12;

    // 상단 높이를 계산한다.
    int nTopH = max(1, (rcClient.Height() - nOuterMargin * 2 - nVerticalGap) / 2);

    // 하단 문의 리스트 Y 좌표이다.
    int nBottomY = nOuterMargin + nTopH + nVerticalGap;

    // 하단 문의 리스트 높이이다.
    int nBottomH = max(1, rcClient.Height() - nBottomY - nOuterMargin);

    // 좌측 차트 영역 너비를 약간 더 좁게 잡아
    // 우측 리스트와 절대 겹치지 않도록 한다.
    int nChartW = max(1, (rcClient.Width() - nOuterMargin * 2 - nHorizontalGap) * 43 / 100);

    // 우측 리스트 시작 X 좌표이다.
    int nDispatchX = nOuterMargin + nChartW + nHorizontalGap;

    // 우측 리스트 너비이다.
    int nDispatchW = max(1, rcClient.Width() - nDispatchX - nOuterMargin);

    // 차트 영역을 좌상단에 배치한다.
    m_rcChart = CRect(
        nOuterMargin,
        nOuterMargin,
        nOuterMargin + nChartW,
        nOuterMargin + nTopH
    );

    // 배차 리스트가 만들어져 있으면 우상단에 배치한다.
    if (::IsWindow(m_listDispatch.GetSafeHwnd()))
    {
        m_listDispatch.MoveWindow(
            nDispatchX,
            nOuterMargin,
            nDispatchW,
            nTopH
        );

        // 컬럼 폭을 현재 크기에 맞게 재조정한다.
        CRect rcDisp;
        m_listDispatch.GetClientRect(&rcDisp);
        int nW = rcDisp.Width() - 4;
        if (nW > 0)
        {
            m_listDispatch.SetColumnWidth(0, static_cast<int>(nW * 0.40));
            m_listDispatch.SetColumnWidth(1, static_cast<int>(nW * 0.35));
            m_listDispatch.SetColumnWidth(2, static_cast<int>(nW * 0.25));
        }
    }

    // 문의 리스트가 만들어져 있으면 하단 전체에 배치한다.
    if (::IsWindow(m_listInquiry.GetSafeHwnd()))
    {
        m_listInquiry.MoveWindow(
            nOuterMargin,
            nBottomY,
            rcClient.Width() - nOuterMargin * 2,
            nBottomH
        );

        // 컬럼 폭을 현재 크기에 맞게 재조정한다.
        CRect rcInq;
        m_listInquiry.GetClientRect(&rcInq);
        int nWI = rcInq.Width() - 4;
        if (nWI > 0)
        {
            m_listInquiry.SetColumnWidth(0, static_cast<int>(nWI * 0.25));
            m_listInquiry.SetColumnWidth(1, static_cast<int>(nWI * 0.75));
        }
    }
}

// 차트 배경과 테두리를 그린다.
void PageHome::DrawChartBackground(CDC* pDC, const CRect& rc)
{
    // 차트 패널 배경을 채운다.
    pDC->FillSolidRect(rc, RGB(245, 247, 250));

    // 테두리를 그린다.
    CPen borderPen(PS_SOLID, 1, RGB(200, 200, 200));
    CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
    CPen* pOldPen = pDC->SelectObject(&borderPen);

    pDC->Rectangle(rc);

    pDC->SelectObject(pOldPen);
    pDC->SelectObject(pOldBrush);
}

// 파이차트를 실제로 그린다.
void PageHome::DrawPieChart(CDC* pDC, const CRect& rc)
{
    // 먼저 차트 패널 배경을 그린다.
    DrawChartBackground(pDC, rc);

    // 범례 및 내부 여백 값이다.
    int nLegendW = 110;
    int nPadding = 16;
    int nTitleH = 20;

    // 실제 원 그래프 영역을 계산한다.
    int nPieLeft = rc.left + nPadding;
    int nPieTop = rc.top + nPadding + nTitleH;
    int nPieRight = rc.right - nLegendW - nPadding;
    int nPieBot = rc.bottom - nPadding;

    // 원 그래프 가능한 너비/높이를 구한다.
    int nPieW = nPieRight - nPieLeft;
    int nPieH = nPieBot - nPieTop;
    int nDiam = min(nPieW, nPieH);

    // 너무 작으면 그리지 않는다.
    if (nDiam < 10)
        return;

    // 반지름과 중심을 계산한다.
    int r = nDiam / 2 - 2;
    int cx = nPieLeft + nPieW / 2;
    int cy = nPieTop + nPieH / 2;

    // 각 슬라이스를 그린다.
    for (int i = 0; i < 5; ++i)
    {
        // 슬라이스 각도를 가져온다.
        double dStart = m_slices[i].startAngle;
        double dEnd = m_slices[i].endAngle;

        // 길이가 너무 짧은 조각은 건너뛴다.
        if ((dEnd - dStart) < 0.001)
            continue;

        // hover 시 약간 튀어나오게 하기 위한 오프셋이다.
        int nOffX = 0;
        int nOffY = 0;

        // 기본 색상이다.
        COLORREF clr = m_slices[i].color;

        // hover 조각이면 강조 표시한다.
        if (i == m_nHoverSlice)
        {
            // 중간각을 기준으로 살짝 이동시킨다.
            double dMid = (dStart + dEnd) / 2.0;
            nOffX = static_cast<int>(cos(dMid) * 10.0);
            nOffY = static_cast<int>(sin(dMid) * 10.0);

            // 색도 조금 밝게 만든다.
            clr = RGB(
                min(255, GetRValue(clr) + 50),
                min(255, GetGValue(clr) + 50),
                min(255, GetBValue(clr) + 50)
            );
        }

        // 브러시와 펜을 만든다.
        CBrush brush(clr);
        CPen pen(
            PS_SOLID,
            (i == m_nHoverSlice) ? 2 : 1,
            (i == m_nHoverSlice) ? RGB(40, 40, 40) : RGB(160, 160, 160)
        );

        // GDI 객체를 선택한다.
        CBrush* pOldBrush = pDC->SelectObject(&brush);
        CPen* pOldPen = pDC->SelectObject(&pen);

        // 시작점과 끝점을 계산한다.
        int x1 = cx + nOffX + static_cast<int>(cos(dStart) * r);
        int y1 = cy + nOffY + static_cast<int>(sin(dStart) * r);
        int x2 = cx + nOffX + static_cast<int>(cos(dEnd) * r);
        int y2 = cy + nOffY + static_cast<int>(sin(dEnd) * r);

        // 실제 파이 조각을 그린다.
        // GDI Pie()는 반시계 방향으로 그리므로 x2,y2를 먼저 전달한다.
        pDC->Pie(
            cx - r + nOffX,
            cy - r + nOffY,
            cx + r + nOffX,
            cy + r + nOffY,
            x2, y2,
            x1, y1
        );

        // 원래 GDI 객체를 복원한다.
        pDC->SelectObject(pOldBrush);
        pDC->SelectObject(pOldPen);
    }

    // 범례를 오른쪽에 그린다.
    int nLegendX = rc.right - nLegendW + 6;
    int nLegendY = rc.top + (rc.Height() - 5 * 22) / 2;
    if (nLegendY < rc.top + 4)
        nLegendY = rc.top + 4;

    // 범례용 점수 라벨이다.
    LPCTSTR arrStar[5] = { _T("5"), _T("4"), _T("3"), _T("2"), _T("1") };

    // 범례 폰트를 만든다.
    CFont font;
    font.CreatePointFont(90, _T("맑은 고딕"));
    CFont* pOldFont = pDC->SelectObject(&font);

    // 텍스트 배경을 투명으로 설정한다.
    pDC->SetBkMode(TRANSPARENT);

    // 범례 항목들을 그린다.
    for (int i = 0; i < 5; ++i)
    {
        // 현재 범례 Y 좌표이다.
        int nY = nLegendY + i * 22;

        // 범위 밖이면 종료한다.
        if (nY + 14 > rc.bottom)
            break;

        // 색상 박스를 그린다.
        CBrush br(m_slices[i].color);
        CPen pn(PS_SOLID, 1, RGB(100, 100, 100));
        CBrush* pOldBr = pDC->SelectObject(&br);
        CPen* pOldPn = pDC->SelectObject(&pn);

        pDC->Rectangle(nLegendX, nY, nLegendX + 12, nY + 12);

        pDC->SelectObject(pOldBr);
        pDC->SelectObject(pOldPn);

        // 라벨 문자열을 만든다.
        CString strLabel;
        strLabel.Format(_T(" %s (%d)"), arrStar[i], m_slices[i].count);

        // hover 중인 항목은 더 진한 색으로 표시한다.
        pDC->SetTextColor((i == m_nHoverSlice) ? RGB(0, 0, 0) : RGB(70, 70, 70));

        // 텍스트 영역을 잡는다.
        CRect rcTxt(nLegendX + 14, nY, rc.right - 2, nY + 18);

        // 텍스트를 그린다.
        pDC->DrawText(strLabel, rcTxt, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }

    // 범례 폰트를 복원한다.
    pDC->SelectObject(pOldFont);

    // 제목을 그린다.
    CFont titleFont;
    titleFont.CreatePointFont(95, _T("맑은 고딕"));
    CFont* pOldTitleFont = pDC->SelectObject(&titleFont);

    pDC->SetTextColor(RGB(60, 60, 60));

    CRect rcTitle(rc.left + 4, rc.top + 3, rc.right - 4, rc.top + nTitleH + 3);
    pDC->DrawText(_T("Review Rating"), rcTitle, DT_CENTER | DT_SINGLELINE);

    pDC->SelectObject(pOldTitleFont);
}

// 리사이즈 처리이다.
void PageHome::OnSize(UINT nType, int cx, int cy)
{
    // 부모 클래스 기본 처리를 수행한다.
    PageBase::OnSize(nType, cx, cy);

    // 크기가 바뀌면 내부 레이아웃을 다시 잡는다.
    UpdateLayout();

    // 차트를 다시 그린다.
    Invalidate(FALSE);
}

// 배경 지우기 처리이다.
BOOL PageHome::OnEraseBkgnd(CDC* pDC)
{
    // 현재 클라이언트 영역을 구한다.
    CRect rcClient;
    GetClientRect(&rcClient);

    // 전체 배경을 흰색으로 칠한다.
    pDC->FillSolidRect(rcClient, RGB(255, 255, 255));

    return TRUE;
}

void PageHome::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);

    if (rcClient.IsRectEmpty())
        return;

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);

    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    // 배경을 채운다.
    memDC.FillSolidRect(rcClient, RGB(255, 255, 255));

    // 현재 크기에 맞게 차트/리스트 레이아웃을 다시 계산한다.
    UpdateLayout();

    // 차트 영역이 유효하면 그린다.
    if (!m_rcChart.IsRectEmpty())
        DrawPieChart(&memDC, m_rcChart);

    // 더블 버퍼 결과를 화면에 출력한다.
    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldBmp);
}

// 마우스 이동 처리이다.
void PageHome::OnMouseMove(UINT nFlags, CPoint point)
{
    // 부모 클래스 처리를 수행한다.
    PageBase::OnMouseMove(nFlags, point);

    // 차트 영역이 비어 있거나 현재 포인트가 차트 밖이면 hover를 해제한다.
    if (m_rcChart.IsRectEmpty() || !m_rcChart.PtInRect(point))
    {
        if (m_nHoverSlice != -1)
        {
            m_nHoverSlice = -1;
            InvalidateRect(m_rcChart, FALSE);
        }
        return;
    }

    // 차트 내부 계산용 상수이다.
    int nLegendW = 110;
    int nPadding = 16;
    int nTitleH = 20;

    // 실제 원 그래프 영역을 다시 계산한다.
    int nPieLeft = m_rcChart.left + nPadding;
    int nPieTop = m_rcChart.top + nPadding + nTitleH;
    int nPieRight = m_rcChart.right - nLegendW - nPadding;
    int nPieBot = m_rcChart.bottom - nPadding;

    // 원 그래프 너비/높이/지름을 구한다.
    int nPieW = nPieRight - nPieLeft;
    int nPieH = nPieBot - nPieTop;
    int nDiam = min(nPieW, nPieH);

    // 너무 작으면 계산하지 않는다.
    if (nDiam < 10)
        return;

    // 반지름과 중심을 계산한다.
    int r = nDiam / 2 - 2;
    int cx = nPieLeft + nPieW / 2;
    int cy = nPieTop + nPieH / 2;

    // 현재 포인터 좌표를 중심 기준으로 바꾼다.
    int dx = point.x - cx;
    int dy = point.y - cy;

    // 원 바깥이면 hover를 해제한다.
    if ((dx * dx + dy * dy) > (r * r))
    {
        if (m_nHoverSlice != -1)
        {
            m_nHoverSlice = -1;
            InvalidateRect(m_rcChart, FALSE);
        }
        return;
    }

    // atan2 결과를 0 ~ 2π 범위로 정규화한다.
    double dAngle = NormalizeAngle(atan2(static_cast<double>(dy), static_cast<double>(dx)));

    // 새 hover 인덱스를 찾는다.
    int nNewHover = -1;

    for (int i = 0; i < 5; ++i)
    {
        // 저장된 시작/끝 각도를 같은 기준으로 정규화한다.
        double dStart = NormalizeAngle(m_slices[i].startAngle);
        double dEnd = NormalizeAngle(m_slices[i].endAngle);

        // 일반 구간과 2π를 넘는 구간을 나눠서 판정한다.
        if (dStart <= dEnd)
        {
            if (dAngle >= dStart && dAngle < dEnd)
            {
                nNewHover = i;
                break;
            }
        }
        else
        {
            if (dAngle >= dStart || dAngle < dEnd)
            {
                nNewHover = i;
                break;
            }
        }
    }

    // hover 항목이 바뀌면 차트 부분만 다시 그린다.
    if (nNewHover != m_nHoverSlice)
    {
        m_nHoverSlice = nNewHover;
        InvalidateRect(m_rcChart, FALSE);
    }

    UNREFERENCED_PARAMETER(nFlags);
}

// 마우스 왼쪽 클릭 처리이다.
// 차트 영역 클릭 시 리뷰 페이지로 이동한다.
void PageHome::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 차트 영역 클릭 시 리뷰 페이지로 이동한다.
    if (!m_rcChart.IsRectEmpty() && m_rcChart.PtInRect(point))
    {
        if (m_fnGoReview)
            m_fnGoReview();
    }

    PageBase::OnLButtonDown(nFlags, point);
}

// 문의 리스트 클릭 시 문의 페이지로 이동한다.
void PageHome::OnInquiryClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    // 외부 콜백이 있으면 호출한다.
    if (m_fnGoInquiry)
        m_fnGoInquiry();

    *pResult = 0;
    UNREFERENCED_PARAMETER(pNMHDR);
}

// 배차 리스트 클릭 시 배차 페이지로 이동한다.
void PageHome::OnDispatchClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    // 외부 콜백이 있으면 호출한다.
    if (m_fnGoDispatch)
        m_fnGoDispatch();

    *pResult = 0;
    UNREFERENCED_PARAMETER(pNMHDR);
}

// 배차 상태 컬러링 처리이다.
void PageHome::OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult)
{
    // 커스텀드로우 구조체를 가져온다.
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

    // 기본값은 기본 그리기이다.
    *pResult = CDRF_DODEFAULT;

    // 사전 준비 단계에서는 아이템 단위 알림을 요청한다.
    if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }

    // 아이템 단계에서는 서브아이템 알림을 요청한다.
    if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        return;
    }

    // 상태 컬럼(2번 컬럼)만 색을 바꾼다.
    if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM))
    {
        if (pLVCD->iSubItem == 2)
        {
            // 현재 상태 문자열을 읽는다.
            CString s = m_listDispatch.GetItemText(static_cast<int>(pLVCD->nmcd.dwItemSpec), 2);

            // 상태별 텍스트 색을 바꾼다.
            if (s == _T("완료"))
                pLVCD->clrText = RGB(0, 150, 0);
            else if (s == _T("대기중"))
                pLVCD->clrText = RGB(255, 140, 0);
            else if (s == _T("배송중"))
                pLVCD->clrText = RGB(200, 0, 0);
            else if (s == _T("배차완료"))
                pLVCD->clrText = RGB(128, 0, 128);

            *pResult = CDRF_NEWFONT;
            return;
        }
    }

    // 나머지는 기본 그리기로 둔다.
    *pResult = CDRF_DODEFAULT;
}

BEGIN_MESSAGE_MAP(PageHome, PageBase)
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_NOTIFY(NM_CLICK, INQUIRY_ID, &PageHome::OnInquiryClick)
    ON_NOTIFY(NM_CLICK, DISPATCH_ID, &PageHome::OnDispatchClick)
    ON_NOTIFY(NM_CUSTOMDRAW, DISPATCH_ID, &PageHome::OnCustomDrawDispatch)
END_MESSAGE_MAP()