#pragma once
#include "afxwin.h"

// ================================================================
//  MyMenuPopup.h  ─  My 버튼 클릭 시 나타나는 팝업 메뉴
//
//  [버튼 구성]
//  ┌─────────────┐
//  │ 내정보 수정  │
//  │ 포인트 확인  │
//  │ 로그아웃    │
//  └─────────────┘
//
//  사용법:
//      MyMenuPopup* pPopup = new MyMenuPopup(pParentWnd);
//      pPopup->ShowAt(ptScreen);   // 화면 좌표 기준
// ================================================================

// 팝업에서 부모로 전달하는 메시지
#define WM_MYMENU_SELECTED  (WM_USER + 200)
#define MYMENU_EDIT_INFO    1   // 내정보 수정
#define MYMENU_POINT        2   // 포인트 확인
#define MYMENU_LOGOUT       3   // 로그아웃

class MyMenuPopup : public CWnd
{
    DECLARE_DYNAMIC(MyMenuPopup)
public:
    MyMenuPopup(CWnd* pParent);
    virtual ~MyMenuPopup();

    // ptScreen: 팝업을 띄울 화면 좌표 (버튼 위치 기준)
    void ShowAt(CPoint ptScreen);

protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    CWnd*   m_pParent;
    int     m_nHoverItem = -1;  // 마우스 호버 항목

    static const int ITEM_W = 130;
    static const int ITEM_H = 30;
    static const int ITEM_COUNT = 3;

    struct MenuItem {
        int     id;
        CString label;
    };
    MenuItem m_items[ITEM_COUNT] = {
        { MYMENU_EDIT_INFO, _T("내정보 수정") },
        { MYMENU_POINT,     _T("포인트 확인")    },
        { MYMENU_LOGOUT,    _T("로그아웃")       },
    };

    int HitTest(CPoint pt) const;
};
