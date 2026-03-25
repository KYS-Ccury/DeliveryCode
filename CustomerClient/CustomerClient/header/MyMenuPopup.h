#pragma once
#include "afxwin.h"

// ================================================================
//  MyMenuPopup.h  ─  My 버튼 클릭 시 나타나는 팝업 메뉴
//
//  [버튼 구성]
//  ┌──────────────────┐
//  │ 개인정보 확인     │
//  │ 내정보 수정(PW)  │
//  │ 포인트 확인      │
//  │ 관리자 채팅      │
//  │ 로그아웃         │
//  └──────────────────┘
// ================================================================

#define WM_MYMENU_SELECTED  (WM_USER + 200)
#define MYMENU_MY_INFO      1   // 개인정보 확인
#define MYMENU_EDIT_INFO    2   // 패스워드 변경
#define MYMENU_POINT        3   // 포인트 확인
#define MYMENU_ADMIN_CHAT   4   // 관리자 채팅
#define MYMENU_LOGOUT       5   // 로그아웃

class MyMenuPopup : public CWnd
{
    DECLARE_DYNAMIC(MyMenuPopup)
public:
    MyMenuPopup(CWnd* pParent);
    virtual ~MyMenuPopup();

    void ShowAt(CPoint ptScreen);

protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    CWnd* m_pParent;
    int   m_nHoverItem = -1;

    static const int ITEM_W = 150;
    static const int ITEM_H = 30;
    static const int ITEM_COUNT = 5;

    struct MenuItem { int id; CString label; };
    MenuItem m_items[ITEM_COUNT] = {
        { MYMENU_MY_INFO,    _T("개인정보 확인")   },
        { MYMENU_EDIT_INFO,  _T("패스워드 변경") },
        { MYMENU_POINT,      _T("포인트 확인")     },
        { MYMENU_ADMIN_CHAT, _T("관리자 채팅")     },
        { MYMENU_LOGOUT,     _T("로그아웃")        },
    };

    int HitTest(CPoint pt) const;
};