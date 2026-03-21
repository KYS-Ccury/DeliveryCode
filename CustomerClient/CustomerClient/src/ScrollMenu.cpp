#include "pch.h"                 // 미리 컴파일된 헤더 (컴파일 속도 향상)
#include "ScrollMenu.h"          // 해당 클래스의 선언이 담긴 헤더

// ─────────────────────────────────────────────
//  RTTI(실시간 형식 정보) 설정
// ─────────────────────────────────────────────
// 프로그램 실행 중에 이 객체가 'CScrollMenu' 타입임을 확인할 수 있게 함
IMPLEMENT_DYNAMIC(CScrollMenu, CStatic)

// ─────────────────────────────────────────────
//  생성자 및 소멸자
// ─────────────────────────────────────────────
CScrollMenu::CScrollMenu()
{
    m_nUnitSize = m_nBtnWidth + m_nSpacing; //  각 버튼 하나가 차지하는 너비와 간격을 합친 단위 크기
}

CScrollMenu::~CScrollMenu()
{
    // [메모리 관리] 생성된 버튼 객체들을 하나씩 순회하며 안전하게 삭제
    for (auto pBtn : m_vButtons)
    {
        if (pBtn)
        {
            pBtn->DestroyWindow(); // 윈도우 화면에서 버튼 제거
            delete pBtn;           // 할당된 메모리 해제 (Memory Leak 방지)
        }
    }
}

// ─────────────────────────────────────────────
//  메시지 맵 (이벤트 - 함수 연결)
// ─────────────────────────────────────────────
BEGIN_MESSAGE_MAP(CScrollMenu, CStatic)
    ON_WM_MOUSEMOVE()      // 마우스가 움직일 때 -> OnMouseMove 호출
    ON_WM_LBUTTONDOWN()    // 왼쪽 버튼 누를 때 -> OnLButtonDown 호출
    ON_WM_LBUTTONUP()      // 왼쪽 버튼 뗄 때 -> OnLButtonUp 호출
    ON_WM_MOUSEWHEEL()     // 마우스 휠 굴릴 때 -> OnMouseWheel 호출
    ON_WM_TIMER()          // 타이머 신호 발생 시 -> OnTimer 호출

    // 버튼 클릭 처리 (ID 2000번부터 2100번까지)
    // 여러 개의 버튼 클릭 이벤트를 'OnBtnClicked'라는 하나의 함수에서 통합 관리함
    ON_COMMAND_RANGE(2000, 2100, &CScrollMenu::OnBtnClicked)
END_MESSAGE_MAP()

// ─────────────────────────────────────────────
//  SetMenuItems: 카테고리 버튼들을 화면에 동적으로 생성
// ─────────────────────────────────────────────
void CScrollMenu::SetMenuItems(const std::vector<CString>& items)
{
    // SS_NOTIFY 스타일을 추가해야 Static 컨트롤이 마우스 클릭 이벤트를 감지할 수 있음
    ModifyStyle(0, SS_NOTIFY);

    CRect rect;
    GetClientRect(&rect); // 메뉴바 전체의 크기를 가져옴

    int menuHeight = rect.Height(); // 메뉴바의 실제 세로 높이
    int btnHeight = 40;             // 우리가 만들 버튼의 높이

    // 상단 여백 계산: (전체 높이 - 버튼 높이) / 2
    // 예: (60 - 40) / 2 = 10 -> 위에서 10px 지점부터 시작
    int topMargin = (menuHeight - btnHeight) / 2;

    int startX = m_nLeftMargin; // 첫 번째 버튼이 시작될 왼쪽 여백 (Padding)

    for (int i = 0; i < items.size(); i++)
    {
        // 새로운 버튼 객체를 메모리에 생성 (Heap 영역)
        CButton* pBtn = new CButton();

        // 버튼 생성 (글자, 스타일, 위치, 부모윈도우, 고유 ID 지정)
        // WS_CHILD | WS_VISIBLE: 자식 창으로서 화면에 보이게 함
        // BS_PUSHBUTTON 눌렀을 때 쏙 들어갔다가 떼면 다시 나오는 일반적인 버튼 스타일
        // 
        // CRect 인자 설명:
        // Left: startX
        // Top: topMargin (계산된 여백)
        // Right: startX + m_nBtnWidth
        // Bottom: topMargin + btnHeight (시작점 + 버튼높이)
        //
        // 2000 + i: 2000번부터 시작하는 고유 번호 부여
        pBtn->Create(items[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            CRect(startX, topMargin, startX + m_nBtnWidth, topMargin + btnHeight), this, 2000 + i);

        // 나중에 관리(삭제 등)하기 위해 벡터(배열)에 저장
        m_vButtons.push_back(pBtn);

        // 다음 버튼이 그려질 위치를 계산 (너비 + 간격만큼 옆으로 이동)
        startX += m_nUnitSize;
    }
}

// ─────────────────────────────────────────────
//  OnLButtonDown: 사용자가 메뉴바를 마우스로 꾹 눌렀을 때 (드래그 시작)
// ─────────────────────────────────────────────
void CScrollMenu::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_bDragging = true;      // "지금부터 드래그 중이다"라고 상태 변경
    m_ptLastMouse = point;   // 드래그 거리를 계산하기 위해 현재 누른 지점을 기록

    SetCapture();            // 마우스 커서가 메뉴바 영역을 벗어나도 계속 추적하도록 설정

    // 부모 클래스(CStatic)의 기본 동작도 함께 수행
    CStatic::OnLButtonDown(nFlags, point);
}


// ─────────────────────────────────────────────
//  OnMouseMove: 마우스를 움직일 때 버튼들을 이동시킴 (드래그 시 동작)
// ─────────────────────────────────────────────
void CScrollMenu::OnMouseMove(UINT nFlags, CPoint point)
{
    // 드래그 상태이고, 마우스 왼쪽 버튼이 눌려 있는 경우에만 실행
    if (m_bDragging && (nFlags & MK_LBUTTON))
    {
        // 마우스가 지난번 위치에서 얼마나 움직였는지 차이(Delta) 계산
        int deltaX = point.x - m_ptLastMouse.x;
        m_ptLastMouse = point; // 다음 계산을 위해 현재 위치 저장

        // 데이터를 불러오는 중이거나 오류로 인해 버튼이 생성되지 않았을 때, 강제로 버튼 위치를 옮기려고 하면 메모리 참조 오류(Crash) 가 발생하며 프로그램이 꺼져버림
        if (m_vButtons.empty()) return; // 버튼이 없으면 중단

        // ----------------------------------------------------
        // 아래 한계선 로직은 사용감이 좋지 않아 빼버림
        // ----------------------------------------------------
        //// 한계선(Boundary) 계산을 위해 첫 번째와 마지막 버튼의 위치 파악
        //CButton* pFirst = m_vButtons.front(); // 가장 왼쪽 버튼
        //CButton* pLast = m_vButtons.back();   // 가장 오른쪽 버튼

        //CRect rF, rL, rectClient;

        //// 가장 왼쪽에 있는 첫 번째 버튼의 위치 파악
        //pFirst->GetWindowRect(&rF); // 모니터 화면 전체 기준의 좌표를 가져와 rF에 저장
        //ScreenToClient(&rF);        // 그 좌표를 '내 메뉴바 내부 기준' 좌표로 변환 (예: x=40)

        //// 가장 오른쪽에 있는 마지막 버튼의 위치 파악
        //pLast->GetWindowRect(&rL);  // 마지막 버튼의 모니터 좌표를 rL에 저장
        //ScreenToClient(&rL);        // 마찬가지로 '내 메뉴바 내부 기준' 좌표로 변환 (예: x=800)

        //GetClientRect(&rectClient); // 메뉴바 전체의 크기 확인

        //// 오른쪽 끝에서 m_nLeftMargin 만큼의 여백을 한계선으로 설정
        //int endMargin = rectClient.right - m_nLeftMargin;

        //// 메뉴가 너무 멀리 나가지 않도록 조정
        //// 오른쪽으로 밀 때: 첫 번째 버튼이 설정해둔 왼쪽 여백을 넘지 못하게 함
        //if (deltaX > 0 && rF.left + deltaX > m_nLeftMargin)
        //    deltaX = m_nLeftMargin - rF.left;
        //// 왼쪽으로 밀 때: 마지막 버튼이 오른쪽 한계선(endMargin)보다 안으로 들어오지 못하게 함
        //else if (deltaX < 0 && rL.right + deltaX < endMargin)
        //    deltaX = endMargin - rL.right;
        // ----------------------------------------------------

        // 실제로 버튼들을 이동시킴
        if (deltaX != 0)
        {
            for (auto pBtn : m_vButtons)
            {
                CRect r;
                pBtn->GetWindowRect(&r); ScreenToClient(&r);

                // SetWindowPos: 버튼의 위치를 새 좌표(현재 위치 + 이동량)로 옮김
                // SWP_NOSIZE: 크기는 변경하지 않음
                // SWP_NOZORDER: 앞뒤 순서는 유지함
                // SWP_NOCOPYBITS: 화면의 복사본 사용금지(드래그시의 잔상예방책)
                pBtn->SetWindowPos(NULL, r.left + deltaX, r.top, 0, 0,
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);
            }
            // 화면을 다시 그리도록 요청
            Invalidate();
        }
    }

    // 부모 클래스(CStatic)의 기본 마우스 이동 처리 호출
    CStatic::OnMouseMove(nFlags, point);
}

// ─────────────────────────────────────────────
//  OnLButtonUp: 마우스 왼쪽 버튼을 뗐을 때 (드래그 종료)
// ─────────────────────────────────────────────
void CScrollMenu::OnLButtonUp(UINT nFlags, CPoint point)
{
    // 드래그 중이었다면 종료 절차 진행
    if (m_bDragging)
    {
        m_bDragging = false;    // 드래그 상태 해제
        ReleaseCapture();       // 마우스 제어권 반납 (다른 윈도우도 마우스 인식 가능하게 함)

        // 메뉴가 화면 밖으로 나갔는지 확인하고, 필요하다면 제자리로 돌려보냄
        StartSnapAnimation();
    }

    // 부모 클래스(CStatic)의 기본 클릭 해제 동작 호출
    CStatic::OnLButtonUp(nFlags, point);
}

// ─────────────────────────────────────────────
//  StartSnapAnimation: 드래그 종료 후 메뉴를 제자리로 돌려보낼지 결정
// ─────────────────────────────────────────────
void CScrollMenu::StartSnapAnimation() {
    // 예외 처리: 생성된 버튼이 하나도 없다면 계산할 필요 없이 종료
    if (m_vButtons.empty()) return;

    CRect rFirst, rLast, rectClient;

    // 현재 양 끝 버튼의 위치와 메뉴바 전체 크기를 파악
    m_vButtons.front()->GetWindowRect(&rFirst); ScreenToClient(&rFirst); // 첫 번째 버튼 위치
    m_vButtons.back()->GetWindowRect(&rLast);   ScreenToClient(&rLast);  // 마지막 버튼 위치
    GetClientRect(&rectClient);                                          // 메뉴바 전체 영역

    // 기준이 되는 좌우 여백(마진) 설정
    int startMargin = m_nLeftMargin;               // 왼쪽 시작 한계선 (예: 40)
    int endMargin = rectClient.right - m_nLeftMargin; // 오른쪽 끝 한계선 (너비 - 40)

    bool bNeedAnimation = false; // 애니메이션을 틀어서 제자리로 돌려보내야 하는가? 를 결정하는 스위치

    // 너무 오른쪽으로 밀려서 왼쪽 여백이 휑하게 남았을 때
    if (rFirst.left > startMargin) {
        m_nTargetX = startMargin; // 첫 버튼을 다시 시작 지점으로 보냄
        bNeedAnimation = true;
    }
    // 너무 왼쪽으로 밀려서 마지막 버튼 뒤에 빈 공간이 생겼을 때
    else if (rLast.right < endMargin) {
        // 마지막 버튼의 오른쪽 끝이 endMargin에 딱 붙도록 목표 지점(m_nTargetX) 계산
        int totalWidth = (int)(m_vButtons.size() - 1) * m_nUnitSize + m_nBtnWidth;
        m_nTargetX = endMargin - totalWidth;
        bNeedAnimation = true;
    }
    // 정상 범위 안에 있을 때
    else {
        // 버튼이 잘리지 않고 안전하게 범위 안에 있다면, 그 자리에 그대로 멈춤
        // (애니메이션 플래그를 켜지 않고 함수를 종료함)
        return;
    }

    // 경계를 이탈한 것이 확인되었다면, 애니메이션 타이머(1번) 가동
    // (이 타이머가 OnTimer를 호출하여 버튼들을 m_nTargetX까지 부드럽게 이동시킴)
    if (bNeedAnimation) {
        SetTimer(1, 10, NULL); // 인자의 뜻 : 1번타이머, 0.01초, 이 클래스가 직접 처리
    }
}

// ─────────────────────────────────────────────
//  OnTimer: 설정한 시간(0.01초 등)마다 반복 실행되는 함수
// ─────────────────────────────────────────────
void CScrollMenu::OnTimer(UINT_PTR nIDEvent) {
    // 1번 타이머: 드래그 종료 후 제자리로 돌아가는 애니메이션
    if (nIDEvent == 1) {
        CRect rFirst;
        // 현재 첫 번째 버튼의 위치를 파악하여 목표 지점(m_nTargetX)까지 얼마나 남았는지 계산
        m_vButtons.front()->GetWindowRect(&rFirst); ScreenToClient(&rFirst);

        int diff = m_nTargetX - rFirst.left; // 남은 거리 = 목표 지점 - 현재 위치

        // [감속 로직] 한 번에 다 가지 않고 남은 거리의 1/4만큼만 이동 (점점 느려지며 부드럽게 멈춤)
        int step = diff / 4;

        // 거리가 너무 가까워져서 정수 나눗셈 결과가 0이 되면, 최소 1픽셀은 움직이도록 보정
        if (step == 0) step = (diff > 0) ? 1 : -1;

        // [종료 조건] 남은 거리가 1픽셀 이하로 매우 가까워지면
        if (abs(diff) <= 1) {
            step = diff;    // 마지막 남은 거리만큼 딱 옮겨주고
            KillTimer(1);   // 애니메이션 끝! 1번 알람을 끔
        }

        // 모든 버튼을 계산된 step(이동량)만큼 동시에 이동시킴
        for (auto pBtn : m_vButtons) {
            CRect r; pBtn->GetWindowRect(&r); ScreenToClient(&r);
            pBtn->SetWindowPos(NULL, r.left + step, r.top, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);
        }

        Invalidate(); // 이동한 모습을 화면에 즉시 다시 그림
    }

    // [Case 2] 2번 타이머: 마우스 휠 스크롤이 멈춘 후 '자동 정렬' 대기
    else if (nIDEvent == 2) {
        KillTimer(2);           // 2번 알람은 한 번만 울리면 되므로 즉시 끔
        StartSnapAnimation();   // 휠 이동이 멈췄으니 경계선을 넘었는지 확인하고 정렬 실행
    }

    // 위 마우스 드래그 및 스크롤 휠 차이는 드래그는 '실시간 추적'이고 휠은 '순간 이동' 방식이라 생김

    // 부모 클래스의 기본 타이머 처리 호출
    CStatic::OnTimer(nIDEvent);
}

// ─────────────────────────────────────────────
//  OnBtnClicked: 메뉴바의 개별 버튼을 클릭했을 때 실행되는 함수
// ─────────────────────────────────────────────
void CScrollMenu::OnBtnClicked(UINT nID) {
    // GetParent(): 이 메뉴바를 화면에 띄워준 '부모 창(MainHomeDlg 등)'을 찾습니다.
    // PostMessage(): 부모 창의 우체통에 "알림 메시지"를 던집니다.
    //  - WM_SCROLL_MENU_CLICKED: "스크롤 메뉴에서 버튼이 눌렸다"라는 신호
    //  - nID: "몇 번째 버튼(ID)"이 눌렸는지에 대한 정보 (예: 2000, 2001...)
    //  - 0: 추가로 보낼 데이터 (여기서는 필요 없으므로 0)

    GetParent()->PostMessage(WM_SCROLL_MENU_CLICKED, nID, 0);
}

// ─────────────────────────────────────────────
//  OnMouseWheel: 마우스 휠을 굴릴 때 메뉴를 좌우로 이동
// ─────────────────────────────────────────────
BOOL CScrollMenu::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // 버튼이 없으면 휠 동작 무시
    if (m_vButtons.empty()) return FALSE;

    // 휠 방향에 따른 이동 거리 설정 (양수: 오른쪽, 음수: 왼쪽)
    int deltaX = (zDelta > 0) ? 50 : -50;

    // 실제로 모든 버튼 이동 (OnMouseMove와 동일한 로직)
    if (deltaX != 0) {
        for (auto pBtn : m_vButtons) {
            CRect r;
            pBtn->GetWindowRect(&r); ScreenToClient(&r);

            pBtn->SetWindowPos(NULL, r.left + deltaX, r.top, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);
        }

        // 화면을 즉시 갱신하여 부드러운 움직임 보장
        Invalidate();
        UpdateWindow();

        // 휠 조작이 멈추고 0.2초 뒤에 '스냅 애니메이션' 실행
        // 사용자가 휠을 마구 굴리는 중에는 타이머가 계속 초기화되다가, 굴리기를 멈추는 순간 
        // StartSnapAnimation()이 호출되어 경계선을 정리
        KillTimer(2);
        SetTimer(2, 200, NULL);
    }

    return TRUE;
}