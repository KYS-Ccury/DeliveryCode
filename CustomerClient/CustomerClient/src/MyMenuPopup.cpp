// ================================================================
//  MyMenuPopup.cpp  ─  My 버튼 팝업 메뉴
// ================================================================
#include "pch.h"
#include "MyMenuPopup.h"

IMPLEMENT_DYNAMIC(MyMenuPopup, CWnd)

MyMenuPopup::MyMenuPopup(CWnd* pParent)
    : m_pParent(pParent)
{
}

MyMenuPopup::~MyMenuPopup() {}

BEGIN_MESSAGE_MAP(MyMenuPopup, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_KILLFOCUS()
    ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void MyMenuPopup::ShowAt(CPoint ptScreen)
{
    // 팝업 윈도우 등록 및 생성
    WNDCLASS wc = {};
    wc.lpfnWndProc = ::DefWindowProc;
    wc.hInstance = AfxGetInstanceHandle();
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = _T("MyMenuPopupClass");
    wc.style = CS_SAVEBITS;
    ::RegisterClass(&wc);   // 이미 등록되어 있으면 무시됨

    int totalH = ITEM_H * ITEM_COUNT + 2; // 테두리 포함

    // 항상 전달된 Y 좌표(버튼 top) 기준 위쪽으로 팝업 표시
    int cx = ::GetSystemMetrics(SM_CXSCREEN);
    ptScreen.y -= totalH;   // 버튼 바로 위에 붙임

    // 화면 왼쪽 경계 보정
    if (ptScreen.x + ITEM_W > cx) ptScreen.x = cx - ITEM_W;
    if (ptScreen.x < 0) ptScreen.x = 0;
    if (ptScreen.y < 0) ptScreen.y = 0;

    CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        _T("MyMenuPopupClass"), _T(""),
        WS_POPUP | WS_BORDER,
        ptScreen.x, ptScreen.y, ITEM_W, totalH,
        m_pParent ? m_pParent->GetSafeHwnd() : nullptr,
        nullptr);

    ShowWindow(SW_SHOW);
    SetFocus();      // KillFocus로 자동 닫힘 처리
}

void MyMenuPopup::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    for (int i = 0; i < ITEM_COUNT; ++i) {
        CRect rcItem(0, i * ITEM_H, ITEM_W, (i + 1) * ITEM_H);

        // 호버 강조
        if (i == m_nHoverItem) {
            dc.FillSolidRect(rcItem, RGB(230, 244, 255));
        }
        else {
            dc.FillSolidRect(rcItem, RGB(255, 255, 255));
        }

        // 구분선 (마지막 제외)
        if (i < ITEM_COUNT - 1) {
            CPen pen(PS_SOLID, 1, RGB(220, 220, 220));
            CPen* pOld = dc.SelectObject(&pen);
            dc.MoveTo(8, (i + 1) * ITEM_H);
            dc.LineTo(ITEM_W - 8, (i + 1) * ITEM_H);
            dc.SelectObject(pOld);
        }

        // 텍스트
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(RGB(40, 40, 40));
        CFont font;
        font.CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            HANGUL_CHARSET, 0, 0, 0, 0, _T("맑은 고딕"));
        CFont* pOldFont = dc.SelectObject(&font);

        CRect rcText = rcItem;
        rcText.DeflateRect(12, 0);
        dc.DrawText(m_items[i].label, rcText,
            DT_SINGLELINE | DT_VCENTER | DT_LEFT);
        dc.SelectObject(pOldFont);
    }
}

int MyMenuPopup::HitTest(CPoint pt) const
{
    if (pt.x < 0 || pt.x >= ITEM_W) return -1;
    int idx = pt.y / ITEM_H;
    return (idx >= 0 && idx < ITEM_COUNT) ? idx : -1;
}

void MyMenuPopup::OnMouseMove(UINT, CPoint point)
{
    int newHover = HitTest(point);
    if (newHover != m_nHoverItem) {
        m_nHoverItem = newHover;
        Invalidate();
    }
}

void MyMenuPopup::OnLButtonDown(UINT, CPoint point)
{
    int idx = HitTest(point);
    if (idx >= 0 && m_pParent) {
        int id = m_items[idx].id;
        DestroyWindow();
        // 부모에게 선택 알림
        ::PostMessage(m_pParent->GetSafeHwnd(),
            WM_MYMENU_SELECTED, (WPARAM)id, 0);
    }
    else {
        DestroyWindow();
    }
}

void MyMenuPopup::OnKillFocus(CWnd*)
{
    // 포커스 잃으면 자동으로 닫힘
    DestroyWindow();
}