// MenuDetailDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MenuDetailDlg.h"

#include "CartDlg.h" // 장바구니 창


// MenuDetailDlg

IMPLEMENT_DYNAMIC(MenuDetailDlg, CDialogEx)

MenuDetailDlg::MenuDetailDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MENUDETAIL_DLG, pParent)
{

}

MenuDetailDlg::~MenuDetailDlg()
{
}

void MenuDetailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_OPTIONS_D, m_listOptions);
    DDX_Text(pDX, IDC_STATIC_MENU_NAME_D, m_strMenuName);
}


BEGIN_MESSAGE_MAP(MenuDetailDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_BACK, &MenuDetailDlg::OnBnClickedBtnBack)
	ON_BN_CLICKED(IDC_BTN_CART, &MenuDetailDlg::OnBnClickedBtnCart)
END_MESSAGE_MAP()


// MenuDetailDlg

BOOL MenuDetailDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 1. 리스트 컨트롤 스타일 설정 (체크박스 활성화)
    m_listOptions.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

    // 2. 컬럼 추가 (옵션명, 가격)
    m_listOptions.InsertColumn(0, _T("추가 옵션"), LVCFMT_LEFT, 150);
    m_listOptions.InsertColumn(1, _T("가격"), LVCFMT_RIGHT, 80);

    // 3. 데이터 넣기 함수 호출
    InsertOptionData();

    // 메뉴 이름 표시 (전달받은 m_strMenuName 사용)
    SetDlgItemText(IDC_STATIC_MENU_NAME_D, m_strMenuName);

    return TRUE;
}

// 임시로 하드코딩된 데이터를 넣는 예시
void MenuDetailDlg::InsertOptionData()
{
    m_listOptions.DeleteAllItems();

    // 샘플 데이터 1
    int nIndex = m_listOptions.InsertItem(0, _T("치즈 크러스트 추가"));
    m_listOptions.SetItemText(nIndex, 1, _T("3,000원"));

    // 샘플 데이터 2
    nIndex = m_listOptions.InsertItem(1, _T("고구마 무스 추가"));
    m_listOptions.SetItemText(nIndex, 1, _T("2,000원"));

    // 샘플 데이터 3
    nIndex = m_listOptions.InsertItem(2, _T("콜라 1.25L 변경"));
    m_listOptions.SetItemText(nIndex, 1, _T("1,500원"));
}

// 돌아가기 버튼 (이전 화면으로)
void MenuDetailDlg::OnBnClickedBtnBack()
{
	// 현재 창을 닫고 이전 화면(StoreListDlg)으로 돌아갑니다.
	CDialogEx::OnCancel();
}

// 장바구니 아이콘 버튼 (단순히 장바구니 창만 열기)
void MenuDetailDlg::OnBnClickedBtnCart()
{
	CartDlg dlg;
	//dlg.DoModal();
    if (dlg.DoModal() == IDOK) // 장바구니에서 '주문하기'를 눌러 성공했을 때
    {
        // 주문이 완료되었으므로 옵션창도 즉시 닫아서 '매장화면'으로 보냄
        CDialogEx::OnOK();
    }
}

// 담기 버튼 (IDOK) - 데이터를 저장하고 장바구니 창 열기
void MenuDetailDlg::OnOK()
{
    // 장바구니에 데이터 담기 로직 실행

    // 장바구니 창 띄우기
    CartDlg dlg;
    if (dlg.DoModal() == IDOK) // 장바구니에서 '주문하기'를 눌러 성공했을 때
    {
        // 주문이 완료되었으므로 옵션창도 즉시 닫아서 '매장화면'으로 보냄
        CDialogEx::OnOK();
    }
}