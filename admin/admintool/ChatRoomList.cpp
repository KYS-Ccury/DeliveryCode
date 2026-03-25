#include "pch.h"
#include "ChatRoomList.h"

BEGIN_MESSAGE_MAP(CChatRoomList, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

CChatRoomList::CChatRoomList()
    : m_nSelectedIndex(-1)
    , m_nHoverIndex(-1)
    , m_nScrollPos(0)
    , m_nTotalHeight(0)
    , m_bTrackingMouse(false)
{
}

CChatRoomList::~CChatRoomList()
{
}

void CChatRoomList::SetRooms(const std::vector<ChatRoomItem>& rooms)
{
    m_rooms = rooms;
    m_nSelectedIndex = -1;
    m_nHoverIndex = -1;
    m_nScrollPos = 0;
    RecalcTotalHeight();
    UpdateScrollInfo();
    Invalidate(FALSE);
}

void CChatRoomList::AddRoom(const ChatRoomItem& room)
{
    m_rooms.push_back(room);
    RecalcTotalHeight();
    UpdateScrollInfo();
    Invalidate(FALSE);
}

void CChatRoomList::ClearRooms()
{
    m_rooms.clear();
    m_nSelectedIndex = -1;
    m_nHoverIndex = -1;
    m_nScrollPos = 0;
    RecalcTotalHeight();
    UpdateScrollInfo();
    Invalidate(FALSE);
}

int CChatRoomList::GetSelectedIndex() const
{
    return m_nSelectedIndex;
}

const ChatRoomItem* CChatRoomList::GetSelectedRoom() const
{
    if (m_nSelectedIndex >= 0 && m_nSelectedIndex < (int)m_rooms.size())
        return &m_rooms[m_nSelectedIndex];
    return nullptr;
}

void CChatRoomList::UpdateLastMessage(int nIndex, const CString& strMsg)
{
    if (nIndex >= 0 && nIndex < (int)m_rooms.size())
    {
        m_rooms[nIndex].lastMessage = strMsg;
        Invalidate(FALSE);
    }
}

void CChatRoomList::RecalcTotalHeight()
{
    int nCount = (int)m_rooms.size();
    m_nTotalHeight = CARD_MARGIN + nCount * (CARD_HEIGHT + CARD_MARGIN);
}

void CChatRoomList::UpdateScrollInfo()
{
    if (!GetSafeHwnd()) return;

    CRect rc;
    GetClientRect(&rc);

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = m_nTotalHeight;
    si.nPage = (UINT)rc.Height();
    si.nPos = m_nScrollPos;
    SetScrollInfo(SB_VERT, &si, TRUE);
}

int CChatRoomList::HitTest(CPoint pt) const
{
    int nY = pt.y + m_nScrollPos - CARD_MARGIN;

    if (nY < 0) return -1;

    int nIndex = nY / (CARD_HEIGHT + CARD_MARGIN);

    if (nIndex >= 0 && nIndex < (int)m_rooms.size())
    {
        int nCardTop = CARD_MARGIN + nIndex * (CARD_HEIGHT + CARD_MARGIN) - m_nScrollPos;
        int nCardBottom = nCardTop + CARD_HEIGHT;

        if (pt.y >= nCardTop && pt.y <= nCardBottom)
            return nIndex;
    }

    return -1;
}

void CChatRoomList::ScrollToBottom()
{
    if (!GetSafeHwnd()) return;

    CRect rc;
    GetClientRect(&rc);

    int nMax = m_nTotalHeight - rc.Height();
    if (nMax < 0) nMax = 0;

    m_nScrollPos = nMax;
    UpdateScrollInfo();
    Invalidate(FALSE);
}

BOOL CChatRoomList::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CChatRoomList::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    memDC.FillSolidRect(rcClient, RGB(240, 240, 240));

    if (!m_fontTitle.GetSafeHandle())
        m_fontTitle.CreatePointFont(95, _T("맑은 고딕"));

    if (!m_fontMsg.GetSafeHandle())
        m_fontMsg.CreatePointFont(85, _T("맑은 고딕"));

    memDC.SetBkMode(TRANSPARENT);

    int nCardWidth = rcClient.Width() - SIDE_MARGIN * 2;

    for (int i = 0; i < (int)m_rooms.size(); ++i)
    {
        int nTop = CARD_MARGIN + i * (CARD_HEIGHT + CARD_MARGIN) - m_nScrollPos;
        int nBottom = nTop + CARD_HEIGHT;

        if (nBottom < 0) continue;
        if (nTop > rcClient.bottom) break;

        CRect rcCard(SIDE_MARGIN, nTop, SIDE_MARGIN + nCardWidth, nBottom);

        COLORREF clrBg;
        if (i == m_nSelectedIndex)
            clrBg = RGB(200, 220, 255);
        else if (i == m_nHoverIndex)
            clrBg = RGB(230, 235, 245);
        else
            clrBg = RGB(255, 255, 255);

        CBrush brush(clrBg);
        CPen pen(PS_SOLID, 1, RGB(210, 210, 210));
        CBrush* pOldBr = memDC.SelectObject(&brush);
        CPen* pOldPn = memDC.SelectObject(&pen);

        memDC.RoundRect(rcCard, CPoint(CARD_RADIUS, CARD_RADIUS));

        memDC.SelectObject(pOldBr);
        memDC.SelectObject(pOldPn);

        if (i == m_nSelectedIndex)
        {
            CBrush barBrush(RGB(60, 120, 255));
            CRect rcBar(rcCard.left, rcCard.top + 4, rcCard.left + 4, rcCard.bottom - 4);
            memDC.FillRect(&rcBar, &barBrush);
        }

        CFont* pOldFont = memDC.SelectObject(&m_fontTitle);
        memDC.SetTextColor(RGB(30, 30, 30));

        CString strTitle;
        strTitle.Format(_T("%s (%s)"), (LPCTSTR)m_rooms[i].orderId, (LPCTSTR)m_rooms[i].role);

        CRect rcTitle(
            rcCard.left + CARD_PADDING + 4,
            rcCard.top + 8,
            rcCard.right - CARD_PADDING,
            rcCard.top + 28
        );
        memDC.DrawText(strTitle, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

        memDC.SelectObject(&m_fontMsg);
        memDC.SetTextColor(RGB(120, 120, 120));

        CRect rcMsg(
            rcCard.left + CARD_PADDING + 4,
            rcCard.top + 32,
            rcCard.right - CARD_PADDING,
            rcCard.bottom - 6
        );
        memDC.DrawText(m_rooms[i].lastMessage, &rcMsg, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

        memDC.SelectObject(pOldFont);
    }

    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void CChatRoomList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CRect rc;
    GetClientRect(&rc);

    int nMaxPos = m_nTotalHeight - rc.Height();
    if (nMaxPos < 0) nMaxPos = 0;

    switch (nSBCode)
    {
    case SB_LINEUP:        m_nScrollPos -= 30; break;
    case SB_LINEDOWN:      m_nScrollPos += 30; break;
    case SB_PAGEUP:        m_nScrollPos -= rc.Height(); break;
    case SB_PAGEDOWN:      m_nScrollPos += rc.Height(); break;
    case SB_THUMBTRACK:    m_nScrollPos = (int)nPos; break;
    case SB_THUMBPOSITION: m_nScrollPos = (int)nPos; break;
    }

    if (m_nScrollPos < 0) m_nScrollPos = 0;
    if (m_nScrollPos > nMaxPos) m_nScrollPos = nMaxPos;

    SetScrollPos(SB_VERT, m_nScrollPos);
    Invalidate(FALSE);
}

BOOL CChatRoomList::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    CRect rc;
    GetClientRect(&rc);

    int nMaxPos = m_nTotalHeight - rc.Height();
    if (nMaxPos < 0) nMaxPos = 0;

    m_nScrollPos -= zDelta / 3;

    if (m_nScrollPos < 0) m_nScrollPos = 0;
    if (m_nScrollPos > nMaxPos) m_nScrollPos = nMaxPos;

    SetScrollPos(SB_VERT, m_nScrollPos);
    Invalidate(FALSE);

    return TRUE;
}

void CChatRoomList::OnLButtonDown(UINT nFlags, CPoint point)
{
    int nIndex = HitTest(point);

    if (nIndex >= 0 && nIndex < (int)m_rooms.size())
    {
        if (m_nSelectedIndex >= 0 && m_nSelectedIndex < (int)m_rooms.size())
            m_rooms[m_nSelectedIndex].isSelected = false;

        m_nSelectedIndex = nIndex;
        m_rooms[nIndex].isSelected = true;

        Invalidate(FALSE);

        if (m_fnOnSelectChanged)
            m_fnOnSelectChanged(nIndex);
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CChatRoomList::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_bTrackingMouse)
    {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = GetSafeHwnd();
        TrackMouseEvent(&tme);
        m_bTrackingMouse = true;
    }

    int nIndex = HitTest(point);

    if (nIndex != m_nHoverIndex)
    {
        m_nHoverIndex = nIndex;
        Invalidate(FALSE);
    }

    CWnd::OnMouseMove(nFlags, point);
}

void CChatRoomList::OnMouseLeave()
{
    m_bTrackingMouse = false;

    if (m_nHoverIndex != -1)
    {
        m_nHoverIndex = -1;
        Invalidate(FALSE);
    }

    CWnd::OnMouseLeave();
}

void CChatRoomList::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    UpdateScrollInfo();
    Invalidate(FALSE);
}