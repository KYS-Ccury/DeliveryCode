#include "pch.h"
#include "RoundButton.h"

BEGIN_MESSAGE_MAP(CRoundButton, CButton)
    ON_WM_MOUSEMOVE()
    ON_MESSAGE(WM_MOUSELEAVE, &CRoundButton::OnMouseLeave)
END_MESSAGE_MAP()

void CRoundButton::PreSubclassWindow()
{
    ModifyStyle(0, BS_OWNERDRAW);
    CButton::PreSubclassWindow();
}

void CRoundButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    CDC* pDC = CDC::FromHandle(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);

    bool bPressed  = (lpDIS->itemState & ODS_SELECTED) != 0;
    bool bDisabled = (lpDIS->itemState & ODS_DISABLED)  != 0;
    bool bDefault  = (lpDIS->itemState & ODS_DEFAULT)   != 0;

    // 배경색 결정
    COLORREF clrBg = bDisabled ? RGB(180, 220, 210)
                   : bPressed  ? m_clrHover
                   : m_bHover  ? m_clrHover
                   : bDefault  ? RGB(20, 160, 120)  // 기본 버튼 더 진하게
                   : m_clrBg;

    // 둥근 배경
    CBrush brush(clrBg);
    CBrush* pOld = pDC->SelectObject(&brush);
    CPen    pen(PS_SOLID, 0, clrBg);
    CPen*   pOldPen = pDC->SelectObject(&pen);
    pDC->RoundRect(&rc, CPoint(RADIUS * 2, RADIUS * 2));
    pDC->SelectObject(pOld);
    pDC->SelectObject(pOldPen);

    // 텍스트
    CString text;
    GetWindowText(text);

    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(bDisabled ? RGB(160, 200, 190) : m_clrText);

    LOGFONT lf = {};
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    lf.lfHeight = -13;
    lf.lfWeight = FW_MEDIUM;
    wcscpy_s(lf.lfFaceName, L"맑은 고딕");
    CFont font;
    font.CreateFontIndirect(&lf);
    CFont* pOldFont = pDC->SelectObject(&font);

    pDC->DrawText(text, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    pDC->SelectObject(pOldFont);

    // 포커스 표시 (점선 테두리)
    if (lpDIS->itemState & ODS_FOCUS) {
        rc.DeflateRect(3, 3);
        pDC->DrawFocusRect(&rc);
    }
}

void CRoundButton::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_bTracking) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, GetSafeHwnd(), 0 };
        TrackMouseEvent(&tme);
        m_bTracking = true;
    }
    if (!m_bHover) {
        m_bHover = true;
        Invalidate(FALSE);
    }
    CButton::OnMouseMove(nFlags, point);
}

LRESULT CRoundButton::OnMouseLeave(WPARAM, LPARAM)
{
    m_bHover    = false;
    m_bTracking = false;
    Invalidate(FALSE);
    return 0;
}
