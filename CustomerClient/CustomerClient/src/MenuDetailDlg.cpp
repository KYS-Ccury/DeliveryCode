#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MenuDetailDlg.h"
#include "OrderManager.h"

IMPLEMENT_DYNAMIC(MenuDetailDlg, CDialogEx)

MenuDetailDlg::MenuDetailDlg(CWnd* pParent)
    : CDialogEx(IDD_MENUDETAIL_DLG, pParent) {}
MenuDetailDlg::~MenuDetailDlg() {}

void MenuDetailDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_OPTIONS_D,     m_listOptions);
    DDX_Text(pDX,    IDC_STATIC_MENU_NAME_D, m_strMenuName);
}

BEGIN_MESSAGE_MAP(MenuDetailDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK, &MenuDetailDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_CART, &MenuDetailDlg::OnBnClickedBtnCart)
END_MESSAGE_MAP()

BOOL MenuDetailDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetDlgItemText(IDC_STATIC_MENU_NAME_D, m_strMenuName);

    // IDC_STATIC_BASE_PRICE_D : RC에서 "기본 금액: 18,000원" 라벨
    CString strPrice;
    strPrice.Format(_T("%d"), m_menuInfo.price); // MenuInfo::price (O)
    SetDlgItemText(IDC_STATIC_BASE_PRICE_D, strPrice);

    m_listOptions.SetExtendedStyle(
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
    m_listOptions.InsertColumn(0, _T(""), LVCFMT_LEFT,  150);
    m_listOptions.InsertColumn(1, _T(""), LVCFMT_RIGHT,  80);

    InsertOptionData();
    return TRUE;
}

void MenuDetailDlg::InsertOptionData()
{
    m_listOptions.DeleteAllItems();
    int row = 0;
    // MenuInfo::optionGroups -> OptionGroup -> OptionItem (모두 MenuInfo.h 에 있음)
    for (const auto& og : m_menuInfo.optionGroups) {
        for (const auto& oi : og.items) {
            CString strName  = CA2T(oi.optionName.c_str(), CP_UTF8);
            CString strPrice;
            strPrice.Format(_T("+%d"), oi.optionPrice); // OptionItem::optionPrice (O)
            int idx = m_listOptions.InsertItem(row++, strName);
            m_listOptions.SetItemText(idx, 1, strPrice);
        }
    }
    if (row == 0)
        m_listOptions.InsertItem(0, _T(""));
}

void MenuDetailDlg::OnBnClickedBtnCart()
{
    CartItem newItem;
    newItem.menuID    = m_menuInfo.menuID;      // MenuInfo::menuID (O)
    newItem.menuName  = CT2A(m_strMenuName, CP_UTF8);
    newItem.basePrice = m_menuInfo.price;       // MenuInfo::price (O)
    newItem.quantity  = 1;                      // CartItem::quantity (O)
    newItem.storeID   = OrderManager::GetInstance().GetCurrentStoreID(); // CartItem::storeID (O)

    int row = 0;
    for (const auto& og : m_menuInfo.optionGroups) {
        for (const auto& oi : og.items) {
            if (m_listOptions.GetCheck(row)) {
                newItem.selectedOptions.push_back(oi); // CartItem::selectedOptions (O)
                newItem.basePrice += oi.optionPrice;
            }
            ++row;
        }
    }
    newItem.CalculateTotalPrice();

    int storeID = OrderManager::GetInstance().GetCurrentStoreID();
    bool added  = OrderManager::GetInstance().AddToCart(storeID, newItem);
    if (!added) {
        if (AfxMessageBox(_T(""), MB_YESNO) == IDYES) {
            OrderManager::GetInstance().ClearCart();
            OrderManager::GetInstance().AddToCart(storeID, newItem);
        } else return;
    }
    CDialogEx::OnOK();
}

void MenuDetailDlg::OnBnClickedBtnBack() { CDialogEx::OnCancel(); }
void MenuDetailDlg::OnOK() { OnBnClickedBtnCart(); }
