#include "pch.h"
#include "PageBase.h"

IMPLEMENT_DYNAMIC(PageBase, CDialogEx)

// 공통 페이지 생성자이다.
PageBase::PageBase(UINT nIDTemplate, CWnd* pParent)
    : CDialogEx(nIDTemplate, pParent)
{
}

// 공통 페이지 소멸자이다.
PageBase::~PageBase()
{
}

// 페이지를 표시한다.
// 자식 페이지가 부모 영역 안에서 정상적으로 앞으로 올라오도록 처리한다.
void PageBase::ShowPage()
{
    // 페이지를 보이게 한다.
    ShowWindow(SW_SHOW);

    // 페이지를 최상단 형제로 올린다.
    SetWindowPos(&wndTop, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // 표시 직후 다시 그리게 한다.
    Invalidate(FALSE);
    UpdateWindow();
}

// 페이지를 숨긴다.
void PageBase::HidePage()
{
    // 페이지를 숨긴다.
    ShowWindow(SW_HIDE);
}

// 공통 초기화 함수이다.
// 생성 직후는 기본적으로 숨김 상태로 둔다.
BOOL PageBase::OnInitDialog()
{
    // 부모 클래스 초기화를 먼저 수행한다.
    CDialogEx::OnInitDialog();

    // 자식 컨트롤과 페이지 전환 시 깜빡임을 줄이기 위해 스타일을 보정한다.
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // 생성 직후에는 숨겨 둔다.
    ShowWindow(SW_HIDE);

    return TRUE;
}

// 공통 리사이즈 처리 함수이다.
void PageBase::OnSize(UINT nType, int cx, int cy)
{
    // 부모 클래스 기본 처리이다.
    CDialogEx::OnSize(nType, cx, cy);
}

BEGIN_MESSAGE_MAP(PageBase, CDialogEx)
    ON_WM_SIZE()
END_MESSAGE_MAP()