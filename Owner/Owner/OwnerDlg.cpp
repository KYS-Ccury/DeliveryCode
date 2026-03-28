#include "pch.h"
#include "framework.h"
#include "Owner.h"
#include "OwnerDlg.h"
#include "afxdialogex.h"
#include "NetClient.h"

#include "CStoreDlg.h"  // 매장 관리 창 헤더
#include "CSalesDlg.h"  // 매출 관리 창 헤더
#include "CSettingsDlg.h" // 설정 창 헤더
#include "OrderManager.h"
#include "CStyleManager.h" // 폰트 크기	조절 매니저
#include "COrderDetailManager.h" // 주문 상세 영역 관리 매니저
#include "StoreManager.h" // 영업중 or 영업중지 상태 관리 매니저 
#include "InquiryManager.h" // 문의 창
#define WM_USER_NEW_ORDER (WM_USER + 100)
#define TIMER_POLLING_ORDER 1

extern int g_nOwnerId;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


COwnerDlg::COwnerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_OWNER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_bIsListOpen = true;
}

void COwnerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(COwnerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDC_BTN_ORDER_MGR, &COwnerDlg::OnBnClickedBtnOrderMgr)
	ON_BN_CLICKED(IDC_BTN_STORE_MGR, &COwnerDlg::OnBnClickedBtnStoreMgr)
	ON_BN_CLICKED(IDC_BTN_SALES_MGR, &COwnerDlg::OnBnClickedBtnSalesMgr)
	ON_BN_CLICKED(IDC_BTN_SETTINGS, &COwnerDlg::OnBnClickedBtnSettings)
	ON_BN_CLICKED(IDC_BTN_STATUS, &COwnerDlg::OnBnClickedBtnStatus)
	ON_NOTIFY(NM_CLICK, IDC_LIST_ORDER, &COwnerDlg::OnNMClickListOrder)

	ON_BN_CLICKED(IDC_BTN_TIME_PLUS, &COwnerDlg::OnBnClickedBtnTimePlus)   // [+] 버튼
	ON_BN_CLICKED(IDC_BTN_TIME_MINUS, &COwnerDlg::OnBnClickedBtnTimeMinus) // [-] 버튼
	ON_BN_CLICKED(IDC_BTN_ORDER_ACCEPT, &COwnerDlg::OnBnClickedBtnOrderAccept) // [접수] 버튼
	ON_BN_CLICKED(IDC_BTN_ORDER_REJECT, &COwnerDlg::OnBnClickedBtnOrderReject) // [거부] 버튼
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_ORDER, &COwnerDlg::OnCustomdrawListOrder)
	ON_BN_CLICKED(IDC_BTN_INQUIRY, &COwnerDlg::OnBnClickedBtnInquiry) 
	ON_MESSAGE(WM_USER_NEW_ORDER, &COwnerDlg::OnNewOrderReceived)
	ON_WM_TIMER() // 🚨 [추가] 타이머 이벤트 맵핑
	ON_BN_CLICKED(IDC_BTN_PRINT_RECEIPT, &COwnerDlg::OnBnClickedBtnPrintReceipt)
	ON_BN_CLICKED(IDC_BTN_PRINT_DELIVERY, &COwnerDlg::OnBnClickedBtnPrintDelivery)

END_MESSAGE_MAP()

BOOL COwnerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// 1. 메인 리스트 초기화 (매니저 내부에서 컬럼을 생성하므로 한 번만 호출!)
	COrderDetailManager::InitMainList(this);

	// 2. 주문 상세 영역 초기화 (오른쪽 상세 창)
	COrderDetailManager::InitArea(this, m_fontLogo);

	// 3. 변수 및 스타일 적용
	m_bIsListOpen = true;
	m_nLastOrderCount = 0; // 🚨 초기화

	// 30pt는 너무 커서 아래 버튼들을 가릴 수 있습니다. 26pt 정도로 살짝 줄여볼게요.
	CStyleManager::ApplyFont(GetDlgItem(IDC_STATIC_LOGO), m_fontLogo, 26, true, _T("맑은 고딕"));

	m_bIsOpen = false;
	// 4. 초기 영업 상태 세팅
	SetDlgItemText(IDC_BTN_STATUS, _T("영업 시작"));
	OnBnClickedBtnStatus();
	// 백그라운드 알림 소켓 가동! (서버 IP와 포트 입력)
	m_NotifySocket.ConnectForNotify(Server_ip, Server_port);
	SetTimer(TIMER_POLLING_ORDER, 5000, NULL);//  5초마다 TIMER_POLLING_ORDER 이벤트를 발생시키는 타이머
	// 추가: 시작하자마자 주문 리스트 보여주기
	OnBnClickedBtnOrderMgr();

	return TRUE;
}

//  CClientSocket에서 신호가 오면 자동으로 실행되는 함수!
LRESULT COwnerDlg::OnNewOrderReceived(WPARAM wParam, LPARAM lParam)
{
	// 주문이 왔다는 팝업
	MessageBox(_T("새로운 배달 주문이 접수되었습니다!"), _T("신규 주문"), MB_ICONINFORMATION | MB_TOPMOST);

	// 열려있던 닫혀있던 강제로 닫힘 처리 후 다시 열어서 갱신
	m_bIsListOpen = false;
	OnBnClickedBtnOrderMgr(); // 우리가 만들어둔 리스트 갱신 함수 재활용!

	return 0;
}

void COwnerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

void COwnerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR COwnerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void COwnerDlg::OnBnClickedBtnOrderMgr()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_ORDER);
	if (!pList) return;

	if (m_bIsListOpen) // 1. 현재 열려있으면 -> 닫기
	{
		pList->ShowWindow(SW_HIDE);
		m_bIsListOpen = false;
	}
	else // 2. 현재 닫혀있으면 -> 서버에서 데이터 가져와서 채우고 보여주기
	{
		// 리스트 비우기
		pList->DeleteAllItems();

		int currentOwnerId = g_nOwnerId;

		// 서버와 통신하여 주문 데이터 가져오기 (대기 시간 발생)
		std::vector<OrderInfo> data = OrderManager::FetchOrdersFromServer(currentOwnerId);

		// UI 리스트 컨트롤에 파싱된 데이터 삽입
		for (int i = 0; i < (int)data.size(); ++i)
		{
			CString strID;
			strID.Format(_T("%d"), data[i].nID);

			int nIdx = pList->InsertItem(i, strID);
			pList->SetItemText(nIdx, 1, data[i].strMenu);
			pList->SetItemText(nIdx, 2, data[i].strPrice);
			pList->SetItemText(nIdx, 3, data[i].strStatus);
		}

		if (data.empty()) {
			MessageBox(_T("표시할 주문이 없습니다."), _T("알림"), MB_ICONINFORMATION);
		}

		pList->ShowWindow(SW_SHOW);
		m_bIsListOpen = true;
	}

	Invalidate();
	UpdateWindow();
}

// 2. 매장 관리 버튼
void COwnerDlg::OnBnClickedBtnStoreMgr()
{
	CStoreDlg dlg;
	dlg.DoModal();
}

// 3. 매출 관리 버튼
void COwnerDlg::OnBnClickedBtnSalesMgr()
{
	CSalesDlg dlg;
	dlg.DoModal();
}

// 4. 설정 버튼
void COwnerDlg::OnBnClickedBtnSettings()
{
	CSettingsDlg dlg;
	dlg.DoModal(); // 환경 설정 다이얼로그 띄우기
}

// 5. 영업 중지/시작 버튼 토글
void COwnerDlg::OnBnClickedBtnStatus()
{
	// 현재 상태의 "반대" 상태로 변경 시도
	bool bNextStatus = !m_bIsOpen;
	int currentOwnerId = g_nOwnerId;

	// StoreManager를 통해 백그라운드 서버 통신
	if (StoreManager::UpdateStoreStatus(currentOwnerId, bNextStatus)) {

		// 통신 성공 시 내 상태 업데이트
		m_bIsOpen = bNextStatus;

		// 버튼 텍스트 변경 (영업 중이면 버튼은 "영업 중지"로 표시)
		CString strBtnText = m_bIsOpen ? _T("영업 중지") : _T("영업 시작");
		SetDlgItemText(IDC_BTN_STATUS, strBtnText);

		// 안내 메시지
		CString strMsg = m_bIsOpen ? _T("영업이 시작되었습니다! 이제 고객들이 배달을 주문할 수 있습니다.")
			: _T("영업이 중지되었습니다. 매장이 목록에서 숨겨집니다.");
		MessageBox(strMsg, _T("상태 변경 완료"), MB_OK | MB_ICONINFORMATION);

	}
	else {
		MessageBox(_T("상태 변경에 실패했습니다. 서버 연결을 확인해주세요."), _T("통신 오류"), MB_OK | MB_ICONERROR);
	}
}

void COwnerDlg::OnNMClickListOrder(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nRow = pNMItemActivate->iItem;

	if (nRow >= 0) {
		CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_ORDER);

		// 1. 데이터 가져오기
		CString strNo = pList->GetItemText(nRow, 0);
		CString strMenu = pList->GetItemText(nRow, 1);
		CString strPrice = pList->GetItemText(nRow, 2);

		// 2. 우측 상단 요약 정보 업데이트
		SetDlgItemText(IDC_STATIC_DETAIL_TITLE, _T("한집배달 ") + strNo);
		SetDlgItemText(IDC_STATIC_DETAIL_SUMMARY, strMenu + _T(" · ") + strPrice + _T("원"));

		// 3. 우측 상세 리스트 연동 업데이트 (메뉴명 전달)
		COrderDetailManager::UpdateList(this, strMenu);
	}
	*pResult = 0;
}

void COwnerDlg::OnBnClickedBtnTimePlus() {
	COrderDetailManager::AdjustCookTime(this, 5); // 5분 증가
}

void COwnerDlg::OnBnClickedBtnTimeMinus() {
	COrderDetailManager::AdjustCookTime(this, -5); // 5분 감소
}
// [접수] 버튼 클릭 시
void COwnerDlg::OnBnClickedBtnOrderAccept()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_ORDER);

	// 1. 현재 리스트에서 어떤 주문이 선택되었는지 확인
	int nRow = pList->GetNextItem(-1, LVNI_SELECTED);

	if (nRow == -1) {
		MessageBox(_T("접수할 주문을 먼저 선택해 주세요!"), _T("알림"), MB_OK | MB_ICONWARNING);
		return;
	}

	// 2. 리스트의 0번째 컬럼(No)에서 order_id 추출
	CString strOrderId = pList->GetItemText(nRow, 0);
	int orderId = _ttoi(strOrderId);

	// 3. 현재 설정된 조리 시간 가져오기 (예: "25분" -> 숫자 25 추출)
	CString strCookTime;
	GetDlgItemText(IDC_STATIC_COOK_TIME, strCookTime);
	int cookTime = _ttoi(strCookTime);

	// 4. 서버로 수락 요청 (DB 업데이트)
	if (OrderManager::AcceptOrder(orderId, cookTime)) {
		// 성공 시 리스트의 '상태' 컬럼 업데이트
		CString strStatus;
		strStatus.Format(_T("조리 중 (%d분)"), cookTime);
		pList->SetItemText(nRow, 3, strStatus);

		MessageBox(strCookTime + _T(" 조리 주문이 정상 접수되었습니다!"), _T("접수 완료"), MB_OK | MB_ICONINFORMATION);
	}
	else {
		MessageBox(_T("서버 오류로 주문 접수에 실패했습니다."), _T("실패"), MB_OK | MB_ICONERROR);
	}
}

// [거부] 버튼 클릭 시
void COwnerDlg::OnBnClickedBtnOrderReject()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_ORDER);
	int nRow = pList->GetNextItem(-1, LVNI_SELECTED);

	if (nRow >= 0) {
		if (MessageBox(_T("정말로 이 주문을 거부하시겠습니까?"), _T("주문 거부"), MB_YESNO | MB_ICONQUESTION) == IDYES) {

			// 1. 리스트의 0번째 컬럼(No)에서 order_id 추출
			CString strOrderId = pList->GetItemText(nRow, 0);
			int orderId = _ttoi(strOrderId);

			// 2. 취소 사유 (현재는 고정값, 원한다면 별도 다이얼로그로 입력받을 수 있음)
			CString reason = _T("재료 소진 및 가게 사정");

			// 3. 서버로 거절 요청 (DB 업데이트)
			if (OrderManager::RejectOrder(orderId, reason)) {
				// 성공 시 UI에서 항목 삭제
				pList->DeleteItem(nRow);

				// 우측 상세 영역 비우기
				SetDlgItemText(IDC_STATIC_DETAIL_TITLE, _T("선택된 주문 없음"));
				SetDlgItemText(IDC_STATIC_DETAIL_SUMMARY, _T("-"));
				((CListCtrl*)GetDlgItem(IDC_LIST_ORDER_MENU))->DeleteAllItems();

				MessageBox(_T("주문이 거부 처리되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
			}
			else {
				MessageBox(_T("서버 오류로 주문 거부에 실패했습니다."), _T("실패"), MB_OK | MB_ICONERROR);
			}
		}
	}
	else {
		MessageBox(_T("거부할 주문을 먼저 선택해 주세요!"), _T("알림"), MB_OK | MB_ICONWARNING);
	}
}

void COwnerDlg::OnCustomdrawListOrder(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	*pResult = CDRF_DODEFAULT;

	// 1. 그리기 단계 확인 (아이템을 그리기 전 단계)
	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		// 2. 현재 줄(Row)의 '상태' 데이터 가져오기 (인덱스 3)
		int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
		CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_ORDER);
		CString strStatus = pList->GetItemText(nItem, 3);

		// 3. "조리 중"이라는 글자가 포함되어 있으면 빨간색으로!
		if (strStatus.Find(_T("조리 중")) != -1)
		{
			pLVCD->clrText = RGB(255, 0, 0);       // 글자색: 빨간색
			// pLVCD->clrTextBk = RGB(255, 240, 240); // 배경색: 아주 연한 분홍색 (선택사항)
		}
		else if (strStatus == _T("완료"))
		{
			pLVCD->clrText = RGB(128, 128, 128);   // 완료된 건 회색으로 흐리게
		}

		*pResult = CDRF_DODEFAULT;
	}
}

void COwnerDlg::OnBnClickedBtnInquiry()
{
	// 로직은 내가(OwnerDlg) 안 짜고 매니저한테 넘긴다!
	CInquiryManager::HandleInquiryClick(this);
}

void COwnerDlg::RefreshOrderListSilently()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_ORDER);
	if (!pList || !m_bIsListOpen) return;

	int currentOwnerId = g_nOwnerId;
	std::vector<OrderInfo> data = OrderManager::FetchOrdersFromServer(currentOwnerId);

	if ((int)data.size() > m_nLastOrderCount) {
		MessageBeep(MB_ICONINFORMATION); // 새 주문 알림음
	}
	m_nLastOrderCount = (int)data.size();

	bool bNeedUpdate = false;
	int currentCount = pList->GetItemCount();
	int newDataCount = (int)data.size();

	if (currentCount != newDataCount) {
		bNeedUpdate = true; // 개수가 다르면 무조건 갱신
	}
	else {
		for (int i = 0; i < newDataCount; ++i) {
			CString strID;
			strID.Format(_T("%d"), data[i].nID);

			if (pList->GetItemText(i, 0) != strID ||
				pList->GetItemText(i, 3) != data[i].strStatus) {
				bNeedUpdate = true;
				break;
			}
		}
	}

	if (!bNeedUpdate) return;

	CString strSelectedOrderId = _T("");
	int nSelectedRow = pList->GetNextItem(-1, LVNI_SELECTED);
	if (nSelectedRow != -1) {
		strSelectedOrderId = pList->GetItemText(nSelectedRow, 0);
	}

	pList->SetRedraw(FALSE); // 그리기 임시 중지

	for (int i = 0; i < newDataCount; ++i) {
		CString strID;
		strID.Format(_T("%d"), data[i].nID);

		if (i < currentCount) {
			pList->SetItemText(i, 0, strID);
			pList->SetItemText(i, 1, data[i].strMenu);
			pList->SetItemText(i, 2, data[i].strPrice);
			pList->SetItemText(i, 3, data[i].strStatus);
		}
		else {
			int nIdx = pList->InsertItem(i, strID);
			pList->SetItemText(nIdx, 1, data[i].strMenu);
			pList->SetItemText(nIdx, 2, data[i].strPrice);
			pList->SetItemText(nIdx, 3, data[i].strStatus);
		}
	}

	for (int i = currentCount - 1; i >= newDataCount; --i) {
		pList->DeleteItem(i); // 남는 찌꺼기 줄 삭제 (qa 오타 수정됨!)
	}

	// 아까 선택했던 주문 다시 포커스(파란줄) 주기
	if (!strSelectedOrderId.IsEmpty()) {
		for (int i = 0; i < pList->GetItemCount(); ++i) {
			if (pList->GetItemText(i, 0) == strSelectedOrderId) {
				pList->SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				break;
			}
		}
	}

	pList->SetRedraw(TRUE); // 모든 업데이트가 끝나면 화면 그리기
}

// 🚨 [추가] 타이머가 돌 때마다 실행되는 함수
void COwnerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_POLLING_ORDER)
	{
		// 5초마다 조용히 서버를 찔러서 새 데이터를 가져옴
		RefreshOrderListSilently();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void COwnerDlg::OnBnClickedBtnPrintReceipt()
{
	// 주문 전표 재출력 버튼 클릭 시
	COrderDetailManager::ShowPrintReceipt(this);
}

void COwnerDlg::OnBnClickedBtnPrintDelivery()
{
	// 배달 안내 출력 버튼 클릭 시
	COrderDetailManager::ShowDeliveryGuide(this);
}