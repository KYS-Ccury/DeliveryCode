#pragma once
#include <afxwin.h>

// ======================================================
//  CRoundButton
//  BS_OWNERDRAW 버튼 + 둥근 모서리 + 배민 민트 배경
//
//  사용법:
//    1) rc 파일에서 버튼을 PUSHBUTTON 또는 DEFPUSHBUTTON 으로 정의
//    2) Dialog .h 에  CRoundButton m_btn; 선언
//    3) DoDataExchange 에  DDX_Control(pDX, IDC_BTN_XXX, m_btn); 추가
//       → SubclassDlgItem 이 자동으로 OwnerDraw 전환
// ======================================================
class CRoundButton : public CButton {
public:
    CRoundButton() = default;

    // 색상 커스터마이즈
    void SetColors(COLORREF bg, COLORREF text, COLORREF hover) {
        m_clrBg    = bg;
        m_clrText  = text;
        m_clrHover = hover;
    }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;
    virtual void PreSubclassWindow() override;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);

private:
    COLORREF m_clrBg    = RGB(29, 178, 133);   // 배민 민트
    COLORREF m_clrText  = RGB(255, 255, 255);  // 흰 글씨
    COLORREF m_clrHover = RGB(22, 148, 110);   // 호버 어두운 민트
    bool     m_bHover   = false;
    bool     m_bTracking = false;

    static const int RADIUS = 10;  // 모서리 반지름(px)
};
