// MainHomeDlg.cpp : 구현 파일

#include "pch.h"                 // 컴파일 최적화를 위한 헤더
#include "CustomerClient.h"      // 프로그램 시작파일 InitInstance() 함수포함 헤더
#include "afxdialogex.h"         // 클래스에 Ex (Extended) 가 붙은 확장클래스를 사용할 수 있게끔 하기위한 헤더
#include "MainHomeDlg.h"         // 메인파일 헤더
#include "OrderManager.h"

#include "CartDlg.h"             // 장바구니창
#include "StoreListDlg.h"        // 매장화면
#include "OrderHistoryDlg.h"


// MainHomeDlg 대화 상자

// 이 클래스가 CDialogEx로부터 상속받았다는 사실을 프로그램이 실행 중에도 기억하게 만듬
IMPLEMENT_DYNAMIC(MainHomeDlg, CDialogEx)

// ─────────────────────────────────────────────
//  생성자 및 소멸자
// ─────────────────────────────────────────────

// CDialogEx 부모클래스를 가지며 소유자로서 pParent 를 지정하고 또 IDD_MAINHOME_DLG 번 리소스도면을 사용할 것을 선언
// pParent 에 지정된 소유자는 이 창의 부모창이 됨(부모클래스와는 다름)
MainHomeDlg::MainHomeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAINHOME_DLG, pParent)
{

}

MainHomeDlg::~MainHomeDlg()
{
}

// ─────────────────────────────────────────────
//  DoDataExchange
// ─────────────────────────────────────────────

// 리소스 뷰에서 만든 UI 아이템"과 "소스 코드에 선언한 변수"를 서로 붙여주는 역할
void MainHomeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_STOR, m_listStore);     // ?
	DDX_Control(pDX, IDC_STATIC_MENU_BAR, m_wndScrollMenu); // 음식카테고리메뉴바와 ID를 매핑
}

// ─────────────────────────────────────────────
//  BEGIN_MESSAGE_MAP ~ END_MESSAGE_MAP()
// ─────────────────────────────────────────────

// 윈도우 메시지와 함수를 연결
// (사용자의 행동(이벤트)을 어떤 함수가 처리할지 연결)
BEGIN_MESSAGE_MAP(MainHomeDlg, CDialogEx)
	ON_WM_MOUSEWHEEL()       // 사용자가 마우스 휠을 굴릴 때
	ON_WM_CTLCOLOR()         // 컨트롤(버튼, 배경 등)에 색을 더해서 그릴 때
	ON_MESSAGE(WM_SCROLL_MENU_CLICKED, &MainHomeDlg::OnScrollMenuClicked)  // 음식카테고리메뉴의 스크롤바를 사용자가 클릭했을 때

	ON_BN_CLICKED(IDC_BUTTON1, &MainHomeDlg::OnBnClickedButton1) // 장바구니 버튼 클릭
	ON_NOTIFY(NM_CLICK, IDC_LIST_STOR, &MainHomeDlg::OnNMDblclkListStor) // 매장리스트에서 매장클릭
	ON_BN_CLICKED(IDC_BTN_MYPAGE, &MainHomeDlg::OnBnClickedBtnOrderHistory)
END_MESSAGE_MAP()


// ─────────────────────────────────────────────
//  OnInitDialog()
// ─────────────────────────────────────────────

// 창이 메모리에 생성된 직후, 화면에 보이기 바로 직전에 호출되는 초기화 함수
BOOL MainHomeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 깜빡임 방지(WS_CLIPCHILDREN) 설정 + 크기 조절 막기(WS_DLGFRAME 적용, WS_THICKFRAME 제거)
	ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN | WS_DLGFRAME);

	// 화면 중앙으로 정렬
	CenterWindow();



	// 리스트 컨트롤 스타일 설정 (줄 무늬, 행 전체 선택)
	m_listStore.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 컬럼 추가 (가로 600픽셀 기준 배분)
	m_listStore.InsertColumn(0, _T("가게 정보"), LVCFMT_LEFT, 280);
	m_listStore.InsertColumn(1, _T("배달 시간"), LVCFMT_CENTER, 100);
	m_listStore.InsertColumn(2, _T("별점/최소주문"), LVCFMT_RIGHT, 140);
	m_listStore.InsertColumn(3, _T("거리"), LVCFMT_CENTER, 60);

	// 테스트용 매장 데이터 로드
	OrderManager::GetInstance().LoadStoreData();



	// 음식카테고리메뉴바에 들어갈 데이터 목록 가져오기 (std::string -> CString 변환)
	auto rawCategories = OrderManager::GetInstance().GetCategoryList();

	m_vecCategories.clear();
	for (const auto& cat : rawCategories) {
		// CA2T: ANSI 문자열(std::string)을 TCHAR(CString)로 변환해주는 번역기
		m_vecCategories.push_back(CString(CA2T(cat.c_str())));
	}

	// 목록의 첫 번째 아이템으로 초기 화면 로드
	if (!m_vecCategories.empty())
	{
		UpdateStoreListUI(m_vecCategories[0]);
	}

	// 스크롤 메뉴바에 데이터 설정 - 음식카테고리 메뉴(전체, 치킨, 피자 등)
	m_wndScrollMenu.SetMenuItems(m_vecCategories);

	// 추후 배경이나 버튼같은것에 색상을 입히기 위한 준비
	m_brushBack.CreateSolidBrush(RGB(230, 245, 245)); // 배달 앱 특유의 깔끔한 연회색 페인트를 준비
	m_brushWhite.CreateSolidBrush(RGB(255, 255, 255)); // 글자 뒤나 배경을 칠할 하얀색 페인트를 준비

	// 1. GetDlgItem 함수로 IDC_STATIC_BG 컨트롤의 주소를 가져와 "이 영역은 더러워졌으니(Invalid) 다시 그려라"라고 시스템에 요청
	// 2. 시스템은 화면을 다시 그리기 위해 'WM_CTLCOLOR' 메시지를 이 다이얼로그에 보냄
	// 3. 메시지 맵에 등록된 OnCtlColor 함수가 실행되면서 우리가 설정한 배경색(연회색 붓)을 시스템에 전달
	// 4. 결과적으로 해당 컨트롤이 우리가 원하는 색으로 다시 칠해짐
	GetDlgItem(IDC_STATIC_TOP_BG)->Invalidate();  // 주소창 배경
	GetDlgItem(IDC_STATIC_MENU_BAR)->Invalidate(); // 메뉴바 배경


	return TRUE;  // 컨트롤에 대한 포커스를 설정하지 않으면 TRUE를 반환합니다.
}

// ─────────────────────────────────────────────
//  그 외 핸들러
// ─────────────────────────────────────────────

// 휠 핸들러 구현
BOOL MainHomeDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 마우스 커서 아래에 있는 윈도우를 찾음
	// CRect 객체를 하나 만들면 내부적으로 4개의 멤버 변수를 가짐
	// left: 왼쪽 X 좌표
	// top : 위쪽 Y 좌표
	// right : 오른쪽 X 좌표
	// bottom : 아래쪽 Y 좌표
	CRect rect;

	// 화면 전체에서 메뉴 바(자식 창) 가 차지하고 있는 사각형 영역(rect)이 어디인지 좌표를 따옴
	m_wndScrollMenu.GetWindowRect(&rect);

	// pt는 현재 마우스 커서의 위치
	// 지금 마우스가 메뉴 바 영역 안에 들어와 있니? 라고 묻는 조건문
	if (rect.PtInRect(pt))
	{
		// 자식 컨트롤(m_wndScrollMenu)의 OnMouseWheel을 직접 호출
		// 마우스가 메뉴 바 위에 있다면, 본체(MainHomeDlg)가 휠 신호를 처리하지 않고 메뉴 바 객체(m_wndScrollMenu)에게 신호를 직접 전달
		// 이 덕분에 메뉴 바가 다른 영역과 분리되어 마우스 휠로 좌우로 움직일 수 있게 됨
		return m_wndScrollMenu.OnMouseWheel(nFlags, zDelta, pt);
	}

	// 만약 마우스가 메뉴 바 바깥에 있다면 윈도우 기본 동작(아무 일도 안 하거나 본체 스크롤)을 수행하라고 부모 클래스에게 넘김
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

// 어떤 색으로 칠할지 결정해서 윈도우에게 알려주는 함수
// 화면에 보이는 각각의 구성 요소(버튼, 글자, 배경 등)를 그릴 때마다 매번 호출됨
HBRUSH MainHomeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	// 윈도우 표준 색상(회색이나 흰색) 을 hbr에 담아둠
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	int nID = pWnd->GetDlgCtrlID();

	// ID 가 IDC_STATIC_BG (상단 음식카테고리메뉴바)라면!
	if (nID == IDC_STATIC_TOP_BG || nID == IDC_STATIC_MENU_BAR)
	{
		pDC->SetBkColor(RGB(230, 245, 245));        // 그 위에 써진 글자의 배경색도 배경판 색상과 똑같이 맞춤
		return (HBRUSH)m_brushBack.GetSafeHandle(); // 미리 준비해둔 연회색으로 칠할것을 윈도우에게 명령 
	}

	// 그 외 다이얼로그 바닥(DLG)과 다른 정적 텍스트(STATIC)는 모두 하얀색으로!
	if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);                 // 글자 배경을 투명하게
		return (HBRUSH)m_brushWhite.GetSafeHandle(); // 미리 준비해둔 하얀색으로 칠할것을 윈도우에게 명령  
	}

	// 위 경우가 아니면 기본색(hbr) 리턴 후 종료
	return hbr;
}

LRESULT MainHomeDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM lParam)
{
	// 버튼 ID를 2000번부터 시작하도록 설정했기 때문에 2000을 빼서 0, 1 처럼 배열이나 리스트 인덱스로 쓰기 좋게 변경
	int nIndex = (UINT)wParam - 2000;

	// 계산된 인덱스가 우리가 가진 카테고리 목록 범위 안에 있는지 확인
	if (nIndex >= 0 && nIndex < (int)m_vecCategories.size())
	{
		// 인덱스에 해당하는 카테고리 명칭(예: "치킨")을 꺼냄
		CString selCategory = m_vecCategories[nIndex];

		// 해당 카테고리의 가게 목록으로 화면을 새로고침함
		UpdateStoreListUI(selCategory);
	}

	return 0;
}

// ------------------------------------------------------------------------
// 아래는 분석 필요 (아마 없어질 내용)
// 


// 음식카테고리메뉴에 따라 출력되는 매장정보를 가져와 출력
void MainHomeDlg::UpdateStoreListUI(CString categoryName)
{
	// 기존에 리스트에 있던 데이터들을 삭제
	m_listStore.DeleteAllItems();

	// CString(유니코드)을 std::string(ANSI)으로 변환하여 매니저에게 전달
	std::string targetCat = CT2A(categoryName);

	// 싱글톤 매니저로부터 필터링된 가게 리스트를 받아옴
	// (이미 OrderManager에 GetStoresByCategory가 구현되어 있어야 합니다)
	std::vector<StoreInfo> stores = OrderManager::GetInstance().GetStoresByCategory(targetCat);

	// 받아온 데이터를 하나씩 리스트 컨트롤에 꽂아넣음
	for (int i = 0; i < (int)stores.size(); ++i)
	{
		// [0번 컬럼] 상호명 넣기
		// CA2T: std::string -> CString 변환
		int nRow = m_listStore.InsertItem(i, CA2T(stores[i].storeName.c_str()));

		// [1번 컬럼] 예상 배달 시간
		m_listStore.SetItemText(nRow, 1, CA2T(stores[i].deliveryTime.c_str()));

		// [2번 컬럼] 별점/최소주문
		CString strInfo;
		strInfo.Format(_T("★4.9 / %d원"), stores[i].minOrderAmount);
		m_listStore.SetItemText(nRow, 2, strInfo);

		// [3번 컬럼] 거리 (km 붙이기)
		CString strDistance;
		strDistance.Format(_T("%.1fkm"), stores[i].distance);
		m_listStore.SetItemText(nRow, 3, strDistance);
	}
}

void MainHomeDlg::OnBnClickedButton1()
{
	// 1. 장바구니 다이얼로그 객체 생성
	CartDlg dlg;

	// 2. 창 띄우기 (모달 방식)
	// 장바구니에서 '주문하기'를 눌러 IDOK가 반환되면 메인도 정리하거나 추가 로직 수행 가능
	if (dlg.DoModal() == IDOK)
	{
		// 주문이 완료되어 메인으로 돌아왔을 때의 처리 (예: 장바구니 비우기 등)
	}
}

void MainHomeDlg::OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// 클릭한 행(Row)의 인덱스 가져오기 (-1이면 빈 공간 클릭)
	int nIndex = pNMItemActivate->iItem;

	if (nIndex != -1)
	{
		// 1. 선택한 가게의 이름이나 ID 가져오기 (예: 0번 컬럼이 상호명일 때)
		CString strStoreName = m_listStore.GetItemText(nIndex, 0);

		// 2. 매장 상세 다이얼로그 생성
		StoreListDlg dlg;

		// 3. (선택사항) 상세창에 가게 정보 전달
		// dlg.m_strStoreName = strStoreName;

		// 4. 창 띄우기
		dlg.DoModal();
	}

	*pResult = 0;
}

void MainHomeDlg::OnBnClickedBtnOrderHistory()
{
	// 1. 주문현황 다이얼로그 객체 생성
	OrderHistoryDlg dlg;

	// 2. (선택 사항) 필요한 데이터를 미리 넘겨줍니다.
	dlg.m_strOrderNum = _T("20260323-001");
	dlg.m_strShopName = _T("불고기피자 본점");
	dlg.m_nTotalAmount = 21000;

	// 3. 창 띄우기 (Modal 방식)
	dlg.DoModal();
}