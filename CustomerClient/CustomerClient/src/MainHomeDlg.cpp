// MainHomeDlg.cpp : 구현 파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MainHomeDlg.h"


// MainHomeDlg 대화 상자

IMPLEMENT_DYNAMIC(MainHomeDlg, CDialogEx)

MainHomeDlg::MainHomeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAINHOME_DLG, pParent)
{

}

MainHomeDlg::~MainHomeDlg()
{
	// 생성된 버튼 메모리 해제
	for (auto pBtn : m_vCatButtons) {
		if (pBtn) {
			pBtn->DestroyWindow();
			delete pBtn;
		}
	}
	m_vCatButtons.clear();
}

void MainHomeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_STOR, m_listStore);
}


BEGIN_MESSAGE_MAP(MainHomeDlg, CDialogEx)
	// 이 줄을 추가하여 윈도우 메시지와 함수를 연결합니다.
	ON_WM_MOUSEWHEEL()

	ON_WM_LBUTTONDOWN() // 이게 빠져있으면 드래그 시작이 안 됩니다!
	ON_WM_MOUSEMOVE()   // 이게 빠져있으면 움직임 감지가 안 됩니다!
	ON_WM_LBUTTONUP()   // 이게 빠져있으면 드래그가 안 끝납니다!

	ON_WM_CTLCOLOR()

	ON_WM_TIMER() // 추가
END_MESSAGE_MAP()


// MainHomeDlg 메시지 처리기

BOOL MainHomeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//// 다이얼로그 배경을 그릴 때 자식 컨트롤 영역은 제외하고 그리게 설정 (깜빡임 방지)
    ModifyStyle(0, WS_CLIPCHILDREN); 

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	// 사용자가 마우스로 테두리 잡고 크기 조절 못하게 방어
	// 리소스 뷰 UI 쪽에서 테두리가 다이어로그 프레임이라 창 크기 변환은 안 되게 되어 있지만
	// 나중에 실수로 속성창에서 테두리를 건드리더라도 프로그램이 실행될 때는
	// (기능적으로) 무조건 크기 조절이 안 되게 막아주는 안전장치
	ModifyStyle(WS_THICKFRAME, WS_DLGFRAME);

	// 창 크기를 600x1000으로 고정 (픽셀 단위)
	// 화면 중앙에 배치하려면 마지막 인자에 SWP_SHOWWINDOW 등을 조합할 수 있습니다.
	// 위치와 크기 제어 (가장 많이 씀)
	// SWP_NOMOVE: 창의 위치(X, Y)를 무시합니다. 현재 위치를 고수합니다.
	// SWP_NOSIZE: 창의 크기(cx, cy)를 무시합니다. 현재 크기를 고수합니다.
	// SWP_NOZORDER: 순서(hWndInsertAfter)를 무시합니다. 현재 겹침 순서를 유지합니다.
	//SetWindowPos(NULL, 0, 0, 600, 1000, SWP_NOMOVE | SWP_NOZORDER);
	//SetWindowPos(NULL, 0, 0, 600, 1000, SWP_NOZORDER); // NOMOVE를 빼면 0,0 위치로 감
	CenterWindow(); // 그 후 화면 중앙으로 정렬

	// 리스트 컨트롤 스타일 설정 (줄 무늬, 행 전체 선택)
	m_listStore.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 컬럼 추가 (가로 600픽셀 기준 배분)
	m_listStore.InsertColumn(0, _T("가게 정보"), LVCFMT_LEFT, 280);
	m_listStore.InsertColumn(1, _T("배달 시간"), LVCFMT_CENTER, 100);
	m_listStore.InsertColumn(2, _T("별점/최소주문"), LVCFMT_RIGHT, 140);
	m_listStore.InsertColumn(3, _T("거리"), LVCFMT_CENTER, 60);

	// 샘플 데이터 한 줄 넣어보기
	int nIndex = m_listStore.InsertItem(0, _T("마왕족발 대구점"));
	m_listStore.SetItemText(nIndex, 1, _T("20~30분"));
	m_listStore.SetItemText(nIndex, 2, _T("★4.9 / 15,000원"));
	m_listStore.SetItemText(nIndex, 3, _T("0.8km"));


	// 1. 카테고리 이름 배열
	CString categories[] = { _T("전체"), _T("족발/보쌈"), _T("찜/탕"), _T("일식"), _T("치킨"), _T("피자"), _T("중식"), _T("양식") };

	CRect rectBase;
	GetDlgItem(IDC_STATIC_BG)->GetWindowRect(&rectBase);
	ScreenToClient(&rectBase);

	//// ★ 왼쪽 여백을 기존의 2배인 40px로 설정
	//int startX = rectBase.left + 40;

	//// OnInitDialog() 내부 버튼 생성 부분
	//for (int i = 0; i < 8; i++) {
	//	CButton* pBtn = new CButton();

	//	// 버튼 너비 100, 높이 40
	//	// rectBase.top + 5 를 해서 회색 바 안에서 살짝 아래로 내려오게 조절합니다.
	//	int btnTop = rectBase.top + 5;
	//	int btnBottom = btnTop + 40;

	//	pBtn->Create(categories[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
	//		CRect(startX, btnTop, startX + 100, btnBottom),
	//		this, 2000 + i);

	//	m_vCatButtons.push_back(pBtn);

	//	// 간격: 버튼(100) + 여백(30) = 130px
	//	startX += 130;
	//}

	int startMargin = 40;     // 왼쪽 여백
	int btnWidth = 115;       // 버튼 너비를 살짝 키움 (기존 100)
	int gap = 20;             // 버튼 사이 간격을 줄임 (기존 30)
	int unitSize = btnWidth + gap; // 115 + 20 = 135px (이게 스냅 기준이 됩니다)

	int startX = startMargin;

	for (int i = 0; i < 8; i++) {
		CButton* pBtn = new CButton();

		int btnTop = rectBase.top + 5;
		int btnBottom = btnTop + 40;

		// 계산된 startX를 사용
		pBtn->Create(categories[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			CRect(startX, btnTop, startX + btnWidth, btnBottom),
			this, 2000 + i);

		m_vCatButtons.push_back(pBtn);

		// 다음 버튼 위치: 현재 위치 + unitSize
		startX += unitSize;
	}



	m_brushBack.CreateSolidBrush(RGB(230, 245, 245)); // 아주 연한 회색 (배달 앱 느낌)
	m_brushWhite.CreateSolidBrush(RGB(255, 255, 255)); // 완전 하얀색
	GetDlgItem(IDC_STATIC_BG)->Invalidate();

	return TRUE;  // 컨트롤에 대한 포커스를 설정하지 않으면 TRUE를 반환합니다.
}

BOOL MainHomeDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 1. 이동 거리 계산 (휠 한 번에 보통 30px 이동)
	int deltaX = (zDelta > 0) ? 40 : -40; // 조금 더 시원하게 이동하게 40으로 조절
	if (m_vCatButtons.empty()) return FALSE;

	// 2. 현재 버튼 위치 파악
	CButton* pFirst = m_vCatButtons.front();
	CButton* pLast = m_vCatButtons.back();
	CRect rF, rL;
	pFirst->GetWindowRect(&rF); ScreenToClient(&rF);
	pLast->GetWindowRect(&rL); ScreenToClient(&rL);

	// 3. 한계선 체크 (40px ~ 560px로 수정)
	if (deltaX > 0 && rF.left + deltaX > 40) deltaX = 40 - rF.left;
	else if (deltaX < 0 && rL.right + deltaX < 560) deltaX = 560 - rL.right;

	// 4. 실제 이동
	if (deltaX != 0) {
		for (auto pBtn : m_vCatButtons) {
			CRect r;
			pBtn->GetWindowRect(&r); ScreenToClient(&r);
			pBtn->SetWindowPos(NULL, r.left + deltaX, r.top, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);
		}

		// 배경 영역만 새로고침
		CRect rectStatic;
		GetDlgItem(IDC_STATIC_BG)->GetWindowRect(&rectStatic);
		ScreenToClient(&rectStatic);
		InvalidateRect(&rectStatic, TRUE);
		UpdateWindow();

		// --- [휠 중지 감지 및 스냅 시작] ---
		// 휠을 돌리는 동안에는 계속 타이머를 죽이고 다시 생성합니다.
		// 유저가 휠을 멈추면 200ms 후에 애니메이션 로직이 작동합니다.
		KillTimer(2);
		SetTimer(2, 200, NULL);
	}

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

// 카테고리 메뉴가 화면 밖으로 무한히 도망가지 못하도록 하는 한계선
void MainHomeDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDragging && (nFlags & MK_LBUTTON))
	{
		// 1. 마우스 이동량 계산
		int deltaX = point.x - m_ptLastMouse.x;
		m_ptLastMouse = point;

		// [속도 제한] 한 프레임에 너무 많이 움직이지 못하게 락 (잔상 방지 핵심)
		if (deltaX > 25) deltaX = 25;
		if (deltaX < -25) deltaX = -25;

		if (deltaX == 0 || m_vCatButtons.empty()) return;

		// 2. 한계선 체크 (40px ~ 540px)
		CButton* pFirst = m_vCatButtons.front();
		CButton* pLast = m_vCatButtons.back();
		CRect rF, rL;
		pFirst->GetWindowRect(&rF); ScreenToClient(&rF);
		pLast->GetWindowRect(&rL); ScreenToClient(&rL);

		//if (deltaX > 0 && rF.left + deltaX > 40) deltaX = 40 - rF.left;
		//else if (deltaX < 0 && rL.right + deltaX < 540) deltaX = 540 - rL.right;
		
		// 왼쪽 끝 한계: 첫 번째 버튼의 왼쪽이 40을 넘지 못하게
		if (deltaX > 0 && rF.left + deltaX > 40)
			deltaX = 40 - rF.left;

		// 오른쪽 끝 한계: 마지막 버튼의 오른쪽이 560(600-40)보다 작아지지 않게
		else if (deltaX < 0 && rL.right + deltaX < 560)
			deltaX = 560 - rL.right;

		// 3. 실제 이동 및 즉시 갱신
		if (deltaX != 0)
		{
			for (auto pBtn : m_vCatButtons) {
				CRect r;
				pBtn->GetWindowRect(&r); ScreenToClient(&r);

				// SWP_NOCOPYBITS 스타일을 추가하면 잔상이 덜 남습니다.
				pBtn->SetWindowPos(NULL, r.left + deltaX, r.top, 0, 0,
					SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);
			}

			// 4. 버튼 통로(Static) 영역만 강제로 새로고침
			CRect rectStatic;
			GetDlgItem(IDC_STATIC_BG)->GetWindowRect(&rectStatic);
			ScreenToClient(&rectStatic);

			// TRUE로 설정해 배경까지 싹 지우고 다시 그리게 합니다.
			InvalidateRect(&rectStatic, TRUE);
			UpdateWindow();
		}
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void MainHomeDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 1. 클릭한 위치의 자식 윈도우 확인
    CWnd* pWndChild = ChildWindowFromPoint(point);
    
    if (pWndChild && pWndChild != this) 
    {
        // 클릭된 게 배경(IDC_STATIC_BG)이 아니라 '진짜 버튼'인지 확인
        if (pWndChild->GetDlgCtrlID() != IDC_STATIC_BG) 
        {
            // 진짜 버튼(장바구니, 카테고리 버튼 등)이면 드래그 하지 않고 종료
            CDialogEx::OnLButtonDown(nFlags, point);
            return;
        }
    }

    // 2. 배경(IDC_STATIC_BG)을 눌렀거나 빈 공간일 때만 드래그 실행
    CRect rectTarget;
    GetDlgItem(IDC_STATIC_BG)->GetWindowRect(&rectTarget);
    ScreenToClient(&rectTarget);

    if (rectTarget.PtInRect(point)) 
    {
        m_bDragging = true;
        m_ptLastMouse = point;
        SetCapture(); 
    }

    CDialogEx::OnLButtonDown(nFlags, point);
}

//void MainHomeDlg::OnLButtonUp(UINT nFlags, CPoint point)
//{
//	if (m_bDragging)
//	{
//		m_bDragging = false;
//		 ReleaseCapture(); // <-- 여기도 주석 처리!
//	}
//	CDialogEx::OnLButtonUp(nFlags, point);
//}

//void MainHomeDlg::OnLButtonUp(UINT nFlags, CPoint point)
//{
//	if (m_bDragging)
//	{
//		m_bDragging = false;
//		ReleaseCapture();
//
//		if (!m_vCatButtons.empty())
//		{
//			CRect rFirst;
//			m_vCatButtons.front()->GetWindowRect(&rFirst);
//			ScreenToClient(&rFirst);
//
//			int startMargin = 40;
//			int unitSize = 130;
//
//			// 1. 가장 가까운 스냅 지점 계산
//			int offset = rFirst.left - startMargin;
//			int nIndex = (offset >= 0) ? (offset + unitSize / 2) / unitSize : (offset - unitSize / 2) / unitSize;
//
//			m_nTargetX = startMargin + (nIndex * unitSize);
//
//			// 2. 타이머 시작 (10ms 간격으로 호출)
//			if (rFirst.left != m_nTargetX) {
//				SetTimer(1, 10, NULL);
//			}
//		}
//	}
//	CDialogEx::OnLButtonUp(nFlags, point);
//}

void MainHomeDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		m_bDragging = false;
		ReleaseCapture();

		if (!m_vCatButtons.empty())
		{
			CRect rFirst, rLast;
			m_vCatButtons.front()->GetWindowRect(&rFirst); ScreenToClient(&rFirst);
			m_vCatButtons.back()->GetWindowRect(&rLast); ScreenToClient(&rLast);

			int startMargin = 40;
			int endMargin = 560;
			int unitSize = 135;

			// --- [양 끝단 밀어주기 로직 통일] ---
			if (rFirst.left > startMargin) {
				m_nTargetX = startMargin;
			}
			else if (rLast.right < endMargin) {
				int totalWidth = (int)(m_vCatButtons.size() - 1) * unitSize + 115;
				m_nTargetX = endMargin - totalWidth;
			}
			else {
				int offset = rFirst.left - startMargin;
				// 현재 위치에서 가장 가까운 인덱스로 반올림
				int nIndex = (offset <= 0) ? (offset - unitSize / 2) / unitSize : (offset + unitSize / 2) / unitSize;
				m_nTargetX = startMargin + (nIndex * unitSize);
			}

			if (rFirst.left != m_nTargetX) SetTimer(1, 10, NULL);
		}
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

HBRUSH MainHomeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// 1. 버튼이 움직이는 통로(IDC_STATIC_BG)만 기존 색상 유지
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_BG)
	{
		pDC->SetBkColor(RGB(230, 245, 245)); // 글자 배경도 통로 색에 맞춤
		return (HBRUSH)m_brushBack.GetSafeHandle();
	}

	// 2. 그 외 다이얼로그 바닥(DLG)과 다른 정적 텍스트(STATIC)는 모두 하얀색으로!
	if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)m_brushWhite.GetSafeHandle();
	}

	return hbr;
}

void MainHomeDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) // 애니메이션 실행 타이머
	{
		if (m_vCatButtons.empty()) { KillTimer(1); return; }

		CRect rFirst;
		m_vCatButtons.front()->GetWindowRect(&rFirst);
		ScreenToClient(&rFirst);

		int diff = m_nTargetX - rFirst.left;
		int step = diff / 4;
		if (step == 0) step = (diff > 0) ? 1 : -1;

		if (abs(diff) <= 1) {
			step = diff;
			KillTimer(1);
		}

		for (auto pBtn : m_vCatButtons) {
			CRect r;
			pBtn->GetWindowRect(&r); ScreenToClient(&r);
			pBtn->SetWindowPos(NULL, r.left + step, r.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);
		}

		CRect rectStatic;
		GetDlgItem(IDC_STATIC_BG)->GetWindowRect(&rectStatic);
		ScreenToClient(&rectStatic);
		InvalidateRect(&rectStatic, TRUE);
	}
	else if (nIDEvent == 2) // 휠 중지 감지 타이머
	{
		KillTimer(2);

		if (!m_vCatButtons.empty())
		{
			CRect rFirst, rLast;
			m_vCatButtons.front()->GetWindowRect(&rFirst); ScreenToClient(&rFirst);
			m_vCatButtons.back()->GetWindowRect(&rLast); ScreenToClient(&rLast);

			int startMargin = 40;
			int endMargin = 560;
			int unitSize = 135;

			// --- [양 끝단 밀어주기 로직 통일] ---
			if (rFirst.left > startMargin) { // 왼쪽 빈틈
				m_nTargetX = startMargin;
			}
			else if (rLast.right < endMargin) { // 오른쪽 빈틈
				int totalWidth = (int)(m_vCatButtons.size() - 1) * unitSize + 115;
				m_nTargetX = endMargin - totalWidth;
			}
			else { // 중간 지점 스냅
				int offset = rFirst.left - startMargin;
				int nIndex = (offset <= 0) ? (offset - unitSize / 2) / unitSize : (offset + unitSize / 2) / unitSize;
				m_nTargetX = startMargin + (nIndex * unitSize);
			}

			if (rFirst.left != m_nTargetX) SetTimer(1, 10, NULL);
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}