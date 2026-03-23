// CartDlg.cpp : 구현파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "CartDlg.h"

#include "DeliveryOkDlg.h"       // 주문완료창
#include "OptionChangeDlg.h"     // 옵션수량변경창
//#include "CartItem.h"

// CartDlg

IMPLEMENT_DYNAMIC(CartDlg, CDialogEx)

CartDlg::CartDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CART_DLG, pParent)
{

}

CartDlg::~CartDlg()
{
}

void CartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_CART, m_listCart); // 리스트 ID와 변수 연결
}


BEGIN_MESSAGE_MAP(CartDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CartDlg::OnBnClickedOk) // 주문완료창
    ON_BN_CLICKED(IDC_BTN_BACK, &CartDlg::OnBnClickedBtnBack) // 뒤로가기
    ON_BN_CLICKED(IDC_BTN_EDIT_OPTION, &CartDlg::OnBnClickedBtnEditOption)     // 옵션변경 버튼
END_MESSAGE_MAP()


// CartDlg 메시지 핸들러

BOOL CartDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ModifyStyle(WS_CAPTION, 0);
    CenterWindow();

    m_listCart.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // 컬럼 생성 (리소스와 순서 맞춤: 메뉴명, 가격, 수량)
    m_listCart.InsertColumn(0, _T("메뉴명"), LVCFMT_LEFT, 150);
    m_listCart.InsertColumn(1, _T("가격"), LVCFMT_RIGHT, 100);
    m_listCart.InsertColumn(2, _T("수량"), LVCFMT_CENTER, 50);

    // 테스트 데이터 삽입 (m_vecCart 사용)
    CartItem item1;
    item1.menuName = "불고기피자"; // std::string이므로 그냥 대입
    item1.basePrice = 20900;
    item1.quantity = 1;

    m_vecCart.push_back(item1);

    // UI 갱신 함수 호출 (직접 for문 돌리지 말고 만든 함수 사용)
    UpdateCartUI();

    return TRUE;
}

void CartDlg::OnBnClickedOk()
{
    // 1. 주문 완료 다이얼로그 객체 생성
    DeliveryOkDlg okDlg;

    // 2. (선택사항) 장바구니의 최종 정보를 완료 창에 넘겨줄 수 있습니다.
    // okDlg.m_strTotalPrice = _T("21,000원"); 

    // 3. 주문 완료 창 띄우기
    this->ShowWindow(SW_HIDE); // 주문 완료창이 뜰 때 장바구니창은 잠시 숨김

    if (okDlg.DoModal() == IDOK)
    {
        // 주문 완료 창에서 '홈으로' 등을 눌러 닫혔을 때
        CDialogEx::OnOK(); // 장바구니 창도 함께 닫으며 메인으로 전달
    }
    else
    {
        this->ShowWindow(SW_SHOW); // 취소 등으로 돌아오면 다시 장바구니 보여줌
    }
}

// 뒤로가기 함수 구현
void CartDlg::OnBnClickedBtnBack()
{
    // OnCancel()은 창을 닫으면서 DoModal()에 IDCANCEL을 반환합니다.
    // '확인(OK)'이 아닌 '취소/돌아가기'의 의미이므로 OnCancel이 적합합니다.
    CDialogEx::OnCancel();
}


// 옵션수량변경
void CartDlg::OnBnClickedBtnEditOption()
{
    int nIndex = m_listCart.GetSelectionMark();
    if (nIndex == -1) {
        AfxMessageBox(_T("변경할 메뉴를 리스트에서 선택해주세요."));
        return;
    }

    CartItem& selectedItem = m_vecCart[nIndex];

    OptionChangeDlg dlg;
    // std::string을 CString으로 변환하여 전달
    dlg.m_strMenuName = selectedItem.menuName.c_str();
    dlg.m_nQuantity = selectedItem.quantity;
    dlg.m_nBasePrice = selectedItem.basePrice;

    if (dlg.DoModal() == IDOK)
    {
        selectedItem.quantity = dlg.m_nQuantity;
        // UI 갱신 (이미 만들어두신 UpdateCartUI 호출)
        UpdateCartUI();
    }
}

void CartDlg::UpdateCartUI()
{
    m_listCart.DeleteAllItems();
    int totalOrderPrice = 0;

    for (int i = 0; i < (int)m_vecCart.size(); ++i) {
        // 1. 개별 아이템 가격 계산 (수량 반영)
        m_vecCart[i].CalculateTotalPrice();

        // 2. 메뉴명 추가 (0번 컬럼)
        CString strMenuName(m_vecCart[i].menuName.c_str());
        int nRow = m_listCart.InsertItem(i, strMenuName);

        // 3. 가격 추가 (1번 컬럼)
        CString strItemPrice;
        strItemPrice.Format(_T("%d원"), m_vecCart[i].totalPrice);
        m_listCart.SetItemText(nRow, 1, strItemPrice);

        // 4. 수량 추가 (2번 컬럼)
        CString strQty;
        strQty.Format(_T("%d"), m_vecCart[i].quantity);
        m_listCart.SetItemText(nRow, 2, strQty);

        // 전체 합계 누적
        totalOrderPrice += m_vecCart[i].totalPrice;
    }

    // --- 여기서부터 하단 텍스트 UI 업데이트 ---

    // 5. 주문 금액 (항목들의 순수 합계)
    CString strOrderTotal;
    strOrderTotal.Format(_T("%d원"), totalOrderPrice);
    SetDlgItemText(IDC_STATIC_ORDER_PRICE, strOrderTotal);

    // 6. 배달수수료 (고정값 3,000원 예시)
    int deliveryFee = 3000;

    // 7. 총 결제금액 (주문 금액 + 배달비)
    CString strFinalTotal;
    strFinalTotal.Format(_T("%d원"), totalOrderPrice + deliveryFee);
    SetDlgItemText(IDC_STATIC_TOTAL_PRICE, strFinalTotal);

    // 8. 주문하기 버튼 위 텍스트도 업데이트
    CString strBtnText;
    strBtnText.Format(_T("%d원 주문하기"), totalOrderPrice + deliveryFee);
    SetDlgItemText(IDOK, strBtnText);
}