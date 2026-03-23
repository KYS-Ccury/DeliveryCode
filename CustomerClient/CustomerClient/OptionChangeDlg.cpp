// OptionChangeDlg.cpp: 구현 파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OptionChangeDlg.h"


// OptionChangeDlg 대화 상자

IMPLEMENT_DYNAMIC(OptionChangeDlg, CDialogEx)

OptionChangeDlg::OptionChangeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_OPTION_CHANGE, pParent)
{
	m_nQuantity = 1;
}

OptionChangeDlg::~OptionChangeDlg()
{
}

void OptionChangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_OPTIONS, m_listOptions);
}


BEGIN_MESSAGE_MAP(OptionChangeDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_COUNT_MINUS, &OptionChangeDlg::OnBnClickedBtnCountMinus)
    ON_BN_CLICKED(IDC_BTN_COUNT_PLUS, &OptionChangeDlg::OnBnClickedBtnCountPlus)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OPTIONS, &OptionChangeDlg::OnLvnItemchangedListOptions)
END_MESSAGE_MAP()


// OptionChangeDlg 메시지 처리기

BOOL OptionChangeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 1. 리스트 컨트롤 스타일 설정 (체크박스 추가)
    m_listOptions.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listOptions.InsertColumn(0, _T("옵션명"), LVCFMT_LEFT, 150);
    m_listOptions.InsertColumn(1, _T("가격"), LVCFMT_RIGHT, 60);

    // 2. 임시 데이터 삽입 (나중에 DB나 서버에서 받아온 데이터로 대체 가능)
    // 예: 불고기 피자라면?
    int nRow = m_listOptions.InsertItem(0, _T("치즈 크러스트 추가"));
    m_listOptions.SetItemText(nRow, 1, _T("2000"));

    nRow = m_listOptions.InsertItem(1, _T("고구마 무스 추가"));
    m_listOptions.SetItemText(nRow, 1, _T("1500"));

    nRow = m_listOptions.InsertItem(2, _T("음료 1.25L 변경"));
    m_listOptions.SetItemText(nRow, 1, _T("1000"));

    // 기본 정보 세팅
    SetDlgItemText(IDC_STATIC_SELECTED_MENU, m_strMenuName);
    SetDlgItemInt(IDC_EDIT_COUNT, m_nQuantity);

    UpdateTotal();
    return TRUE;
}

void OptionChangeDlg::OnBnClickedBtnCountPlus() {
    m_nQuantity++;
    SetDlgItemInt(IDC_EDIT_COUNT, m_nQuantity);
    UpdateTotal();
}

void OptionChangeDlg::OnBnClickedBtnCountMinus() {
    if (m_nQuantity > 1) {
        m_nQuantity--;
        SetDlgItemInt(IDC_EDIT_COUNT, m_nQuantity);
        UpdateTotal();
    }
}

void OptionChangeDlg::UpdateTotal() {
    int optionTotal = 0;
    int nCount = m_listOptions.GetItemCount();

    for (int i = 0; i < nCount; i++) {
        if (m_listOptions.GetCheck(i)) { // 체크된 항목만 계산
            CString strPrice = m_listOptions.GetItemText(i, 1);
            optionTotal += _ttoi(strPrice); // 문자열을 숫자로 변환
        }
    }

    // 최종 합계 = (기본가 + 옵션총합) * 수량
    int total = (m_nBasePrice + optionTotal) * m_nQuantity;

    CString strTotal;
    strTotal.Format(_T("합계: %d원"), total);
    SetDlgItemText(IDC_STATIC_TOTAL_PRICE_POPUP, strTotal);
}

void OptionChangeDlg::OnLvnItemchangedListOptions(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    // 상태가 변경되었고, 특히 체크박스 상태가 바뀌었을 때만 업데이트
    if (pNMLV->uChanged & LVIF_STATE)
    {
        UpdateTotal();
    }
    *pResult = 0;
}