#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OptionChangeDlg.h"
#include "MenuInfo.h"


IMPLEMENT_DYNAMIC(OptionChangeDlg, CDialogEx)

OptionChangeDlg::OptionChangeDlg(CWnd* pParent)
    : CDialogEx(IDD_DIALOG_OPTION_CHANGE, pParent)
    , m_nQuantity(1)
    , m_nBasePrice(0)
{}

OptionChangeDlg::~OptionChangeDlg() {}

void OptionChangeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_OPTIONS, m_listOptions);
}

BEGIN_MESSAGE_MAP(OptionChangeDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_COUNT_MINUS, &OptionChangeDlg::OnBnClickedBtnCountMinus)
    ON_BN_CLICKED(IDC_BTN_COUNT_PLUS,  &OptionChangeDlg::OnBnClickedBtnCountPlus)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OPTIONS, &OptionChangeDlg::OnLvnItemchangedListOptions)
END_MESSAGE_MAP()

BOOL OptionChangeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_listOptions.SetExtendedStyle(
        LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listOptions.InsertColumn(0, _T("옵션명"), LVCFMT_LEFT,  150);
    m_listOptions.InsertColumn(1, _T("가격"), LVCFMT_RIGHT,  60);

    int row = 0;
    for (const auto& og : m_vecOptionGroups)
    {
        for (const auto& oi : og.items)
        {
            // 2. CA2T 사용 시 유의: optionName이 std::string인 경우
            CString strName = CA2T(oi.optionName.c_str(), CP_UTF8);

            // 3. CA2W 오류 해결: int(optionPrice)를 직접 변환할 수 없으므로 Format 사용
            CString strPrice;
            strPrice.Format(_T("%d"), oi.optionPrice);

            int idx = m_listOptions.InsertItem(row++, strName);
            m_listOptions.SetItemText(idx, 1, strPrice);
        }
    }
    if (row == 0) m_listOptions.InsertItem(0, _T(""));

    // ★ 기존 선택 옵션 체크 상태 복원
    if (!m_vecPreCheckedOptionIDs.empty()) {
        int row2 = 0;
        for (const auto& og : m_vecOptionGroups) {

            for (const auto& oi : og.items) {
                bool bChecked = std::find(
                    m_vecPreCheckedOptionIDs.begin(),
                    m_vecPreCheckedOptionIDs.end(),
                    oi.optionID) != m_vecPreCheckedOptionIDs.end();
                m_listOptions.SetCheck(row2, bChecked ? TRUE : FALSE);
                row2++;
            }
        }
    }

    SetDlgItemText(IDC_STATIC_SELECTED_MENU, m_strMenuName);
    SetDlgItemInt(IDC_EDIT_COUNT, m_nQuantity);
    UpdateTotal();
    return TRUE;
}

void OptionChangeDlg::OnBnClickedBtnCountPlus()
{
    m_nQuantity++;
    SetDlgItemInt(IDC_EDIT_COUNT, m_nQuantity);
    UpdateTotal();
}

void OptionChangeDlg::OnBnClickedBtnCountMinus()
{
    if (m_nQuantity > 1) {
        m_nQuantity--;
        SetDlgItemInt(IDC_EDIT_COUNT, m_nQuantity);
        UpdateTotal();
    }
}

void OptionChangeDlg::UpdateTotal()
{
    int optionTotal = 0;
    int nCount = m_listOptions.GetItemCount();
    for (int i = 0; i < nCount; ++i) {
        if (m_listOptions.GetCheck(i)) {
            CString s = m_listOptions.GetItemText(i, 1);
            optionTotal += _ttoi(s);
        }
    }
    int total = (m_nBasePrice + optionTotal) * m_nQuantity;
    CString str;
    str.Format(_T("%d"), total);
    SetDlgItemText(IDC_STATIC_TOTAL_PRICE_POPUP, str);
}

void OptionChangeDlg::OnLvnItemchangedListOptions(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if (pNMLV->uChanged & LVIF_STATE) UpdateTotal();
    *pResult = 0;
}

void OptionChangeDlg::OnOK()
{
    // ★ 창이 닫히기 전에 체크 상태를 m_vecResultOptionIDs에 저장
    m_vecResultOptionIDs.clear();
    int row = 0;
    for (const auto& og : m_vecOptionGroups) {
        for (const auto& oi : og.items) {
            if (m_listOptions.GetCheck(row))
                m_vecResultOptionIDs.push_back(oi.optionID);
            row++;
        }
    }
    CDialogEx::OnOK();  // 여기서 윈도우 Destroy
}