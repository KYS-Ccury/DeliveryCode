#pragma once                     // 헤더 중복 포함 방지

#include <vector>                // 메뉴 아이템 목록 저장을 위한 std::vector 사용

#define SCROLL_MENU_BTN_ID 5000  // 동적 생성할 버튼 ID (5000 ~ 5100 사용)

// ─────────────────────────────────────────────

//  사용자 정의 메시지 선언

// ─────────────────────────────────────────────

// 부모 윈도우(MainHomeDlg)에게 "어떤 버튼이 클릭되었다"라고 알릴 때 사용할 고유 번호

#define WM_SCROLL_MENU_CLICKED (WM_USER + 100)



// ─────────────────────────────────────────────

//  CScrollMenu 클래스 선언 (CStatic 상속)

// ─────────────────────────────────────────────

class CScrollMenu : public CStatic

{

    // 이 클래스가 실행 중에 자신의 타입 정보를 알 수 있게 함

    DECLARE_DYNAMIC(CScrollMenu)



public:

    // ─────────────────────────────────────────────

    //  생성자 및 소멸자

    // ─────────────────────────────────────────────

    CScrollMenu();

    virtual ~CScrollMenu();



    // ─────────────────────────────────────────────

    //  주요 설정 함수

    // ─────────────────────────────────────────────



    // 메시지가 자식(버튼)에게 가기 전에 부모에게 판단하게 하는 함수

    virtual BOOL PreTranslateMessage(MSG* pMsg);



    // 외부(MainHomeDlg)에서 카테고리 목록(전체, 치킨 등)을 넘겨받아 버튼들을 생성하는 함수

    void SetMenuItems(const std::vector<CString>& items);


protected:

    // ─────────────────────────────────────────────

    //  내부 멤버 변수 (관리용)

    // ─────────────────────────────────────────────

    std::vector<CButton*> m_vButtons;  // 생성된 버튼 객체 목록



    // 드래그 관련 변수

    bool    m_bDragging = false;       // 마우스를 누르고 있는 상태인가?

    bool    m_bIsRealDrag = false;     // 단순 클릭이 아닌 '진짜 드래그'로 판정되었는가?

    CPoint  m_ptLastMouse;             // 실시간 이동량 계산을 위한 직전 좌표

    CPoint  m_ptDown;                  // 드래그 판정 기준점이 되는 최초 클릭 좌표



    // 위치 및 간격 설정값

    int     m_nTargetX = 40;           // 애니메이션 목표 지점

    int     m_nBtnWidth = 180;         // 버튼 너비

    int     m_nSpacing = 0;           // 버튼 간격

    int     m_nUnitSize;               // 단위 크기 (너비 + 간격)

    int     m_nLeftMargin = 0;        // 왼쪽 시작 여백


    // ─────────────────────────────────────────────

    //  내부 로직 함수

    // ─────────────────────────────────────────────

    // 스크롤이 멈췄을 때 버튼이 칸에 딱 맞게 정렬되도록 하는 애니메이션 시작 함수

    void StartSnapAnimation();



    // 메시지 맵 선언 (이벤트 핸들러 연결용)

    DECLARE_MESSAGE_MAP()



protected:

    // ─────────────────────────────────────────────

    //  메시지 핸들러 함수 (이벤트 처리)

    // ─────────────────────────────────────────────



    // 마우스가 메뉴바 위에서 움직일 때 (드래그 처리)

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);



    // 마우스 왼쪽 버튼을 눌렀을 때 (드래그 시작)

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);



    // 마우스 왼쪽 버튼을 뗐을 때 (드래그 종료 및 정렬 시작)

    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);



    // 정해진 시간마다 호출 (애니메이션의 부드러운 움직임을 구현하기 위해 사용)

    afx_msg void OnTimer(UINT_PTR nIDEvent);



    // 동적으로 생성된 버튼이 클릭되었을 때 호출되는 함수

    afx_msg void OnBtnClicked(UINT nID);



    // 배경 지우기 무시 (메뉴바 이동시 점멸방지)

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);



public:

    // 메뉴바 위에서 마우스 휠을 굴렸을 때 (좌우 스크롤 처리)

    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);



};