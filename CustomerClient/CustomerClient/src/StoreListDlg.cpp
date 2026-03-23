// StoreListDlg.cpp : 구현파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "StoreListDlg.h"

#include "OrderManager.h" // 데이터 로드용
#include "CartDlg.h" // 장바구니
#include "StoreDetailDlg.h" // 매장상세화면
#include "MenuDetailDlg.h" // 옵션상세화면

// StoreListDlg

IMPLEMENT_DYNAMIC(StoreListDlg, CDialogEx)

StoreListDlg::StoreListDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STORELIST_DLG, pParent)
{

}

StoreListDlg::~StoreListDlg()
{
}

void StoreListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MENU_ITEMS, m_listMenu);       // 음식 리스트 매핑
	DDX_Control(pDX, IDC_STATIC_SUB_MENU_BAR, m_wndScrollMenu); // 카테고리 바 매핑
}


BEGIN_MESSAGE_MAP(StoreListDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_BACK, &StoreListDlg::OnBnClickedBtnBack)
	ON_WM_MOUSEWHEEL() // 휠 메시지 연결
	ON_MESSAGE(WM_SCROLL_MENU_CLICKED, &StoreListDlg::OnScrollMenuClicked) // 클릭 메시지 연결
    ON_BN_CLICKED(IDC_BTN_CART, &StoreListDlg::OnBnClickedBtnCart) // 장바구니클릭
    ON_BN_CLICKED(IDC_BTN_STORE_INFO, &StoreListDlg::OnBnClickedBtnStoreInfo) // 매장상세화면버튼클릭
    ON_NOTIFY(NM_CLICK, IDC_LIST_MENU_ITEMS, &StoreListDlg::OnNMClickListMenuItems) // 특정메뉴클릭(->옵션선택페이지)
END_MESSAGE_MAP()


// StoreListDlg 메시지 핸들러

BOOL StoreListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 상단바 및 테두리 설정
    ModifyStyle(WS_CAPTION, 0);
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN);
    CenterWindow();

    // 리스트 컨트롤 설정 (음식 목록)
    m_listMenu.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMenu.InsertColumn(0, _T("음식명"), LVCFMT_LEFT, 150);
    m_listMenu.InsertColumn(1, _T("설명 및 가격"), LVCFMT_LEFT, 200);
    m_listMenu.InsertColumn(2, _T("리뷰수"), LVCFMT_CENTER, 80);

    // 스크롤 메뉴 카테고리 설정
    // 여긴 추후 데이터베이스에서 오는걸로 빼야 함
    m_vecSubCategories = { _T("인기메뉴"), _T("세트메뉴"), _T("식사류"), _T("안주류"), _T("음료/주류") };

    // 버튼 생성 전 안전장치: 윈도우가 생성되었는지 확인
    if (m_wndScrollMenu.GetSafeHwnd()) {
        m_wndScrollMenu.SetMenuItems(m_vecSubCategories);
    }

    // 초기 데이터 로드
    UpdateMenuListUI(m_vecSubCategories[0]);

    return TRUE;
}

// 스크롤 메뉴 클릭 시 음식 리스트 필터링
LRESULT StoreListDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM lParam)
{
    int nIndex = (UINT)wParam - 2000;
    if (nIndex >= 0 && nIndex < (int)m_vecSubCategories.size())
    {
        UpdateMenuListUI(m_vecSubCategories[nIndex]);
    }
    return 0;
}

// 음식 리스트를 실제로 채워넣는 함수
void StoreListDlg::UpdateMenuListUI(CString subCategory)
{
    m_listMenu.DeleteAllItems();

    // 실제로는 OrderManager에서 이 가게의 특정 카테고리 음식을 가져와야 함
    // 여기서는 구조를 보여드리기 위해 샘플 코드를 넣습니다.
    if (subCategory == _T("인기메뉴")) {
        int nRow = m_listMenu.InsertItem(0, _T("불고기 피자"));
        m_listMenu.SetItemText(nRow, 1, _T("추천! / 27,900원"));
    }
    else if (subCategory == _T("세트메뉴")) {
        int nRow = m_listMenu.InsertItem(0, _T("피자 세트"));
        m_listMenu.SetItemText(nRow, 1, _T("57,900원"));
    }
}

// 메뉴 바 위에서 휠을 굴리면 가로로 스크롤되게 전달
BOOL StoreListDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    CRect rect;
    m_wndScrollMenu.GetWindowRect(&rect);
    if (rect.PtInRect(pt))
    {
        return m_wndScrollMenu.OnMouseWheel(nFlags, zDelta, pt);
    }
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void StoreListDlg::OnBnClickedBtnBack()
{
    // 현재 상세창을 닫으면 DoModal()이 종료되면서 메인 화면으로 돌아갑니다.
    CDialogEx::OnCancel();
}

void StoreListDlg::OnBnClickedBtnCart()
{
    // 장바구니 다이얼로그 객체 생성
    CartDlg dlg;

    // 필요하다면 현재 선택된 매장 정보나 장바구니 데이터를 전달합니다.
    // dlg.SetCartData(m_pOrderManager->GetCart()); 

    // 모달 창 띄우기
    if (dlg.DoModal() == IDOK)
    {
        // 장바구니에서 '주문하기'를 눌러 성공적으로 돌아온 경우의 처리
        // 예를 들어, 주문 완료 후 메인화면으로 바로 가고 싶다면 여기서 EndDialog(IDOK) 호출
    }
}

void StoreListDlg::OnBnClickedBtnStoreInfo()
{
    StoreDetailDlg dlg;

    // [중요] 상세 화면에 띄울 데이터를 미리 세팅합니다.
    // 실제 프로젝트에서는 현재 선택된 매장 객체에서 데이터를 가져와야 합니다.
    dlg.m_strName = _T("아메리칸 피자 강남점");
    dlg.m_strAddr = _T("서울특별시 강남구 테헤란로 123");
    dlg.m_strTime = _T("매일 11:00 ~ 23:00");
    dlg.m_strOff = _T("연중무휴");
    dlg.m_strTel = _T("02-1234-5678");

    // 상세 창 띄우기
    dlg.DoModal();
}

void StoreListDlg::OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

    // 클릭된 행의 인덱스 (아이템이 없는 빈 공간을 클릭하면 -1 반환)
    int nIndex = pNMItemActivate->iItem;

    // 유효한 아이템을 클릭했을 때만 실행
    if (nIndex != -1)
    {
        // 1. 리스트에서 정보 가져오기
        CString strName = m_listMenu.GetItemText(nIndex, 0); // 첫 번째 컬럼: 메뉴명
        //CString strName = m_listMenuItems.GetItemText(nIndex, 0);

        // 2. 옵션 상세 다이얼로그 생성 및 데이터 전달
        MenuDetailDlg dlg;
        dlg.m_strMenuName = strName;

        // 3. 창 띄우기
        dlg.DoModal();
    }

    *pResult = 0;
}