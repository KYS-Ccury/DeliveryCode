#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "CartDlg.h"
#include "DeliveryOkDlg.h"
#include "OptionChangeDlg.h"
#include "OrderManager.h"

IMPLEMENT_DYNAMIC(CartDlg, CDialogEx)

CartDlg::CartDlg(CWnd* pParent)
    : CDialogEx(IDD_CART_DLG, pParent) {}
CartDlg::~CartDlg() {}

void CartDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_CART, m_listCart);
}

BEGIN_MESSAGE_MAP(CartDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                &CartDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BTN_BACK,        &CartDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_EDIT_OPTION, &CartDlg::OnBnClickedBtnEditOption)
END_MESSAGE_MAP()

BOOL CartDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_CAPTION, 0);
    CenterWindow();

    m_listCart.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listCart.InsertColumn(0, _T(""), LVCFMT_LEFT,   150);
    m_listCart.InsertColumn(1, _T(""), LVCFMT_RIGHT,  100);
    m_listCart.InsertColumn(2, _T(""), LVCFMT_CENTER,  50);

    m_vecCart = OrderManager::GetInstance().GetCartItems();
    UpdateCartUI();
    return TRUE;
}

void CartDlg::UpdateCartUI()
{
    m_listCart.DeleteAllItems();
    int total = 0;

    for (int i = 0; i < (int)m_vecCart.size(); ++i) {
        m_vecCart[i].CalculateTotalPrice();
        CString strName = CA2T(m_vecCart[i].menuName.c_str(), CP_UTF8);
        int nRow = m_listCart.InsertItem(i, strName);
        CString s1; s1.Format(_T("%d"), m_vecCart[i].totalPrice);
        m_listCart.SetItemText(nRow, 1, s1);
        CString s2; s2.Format(_T("%d"), m_vecCart[i].quantity); // CartItem::quantity (O)
        m_listCart.SetItemText(nRow, 2, s2);
        total += m_vecCart[i].totalPrice;
    }

    int fee   = 3000;
    int final = total + fee;
    CString s; s.Format(_T("%d"), total);  SetDlgItemText(IDC_STATIC_ORDER_PRICE, s);
    s.Format(_T("%d"), final);             SetDlgItemText(IDC_STATIC_TOTAL_PRICE, s);
    s.Format(_T("%d"), final);             SetDlgItemText(IDOK, s);
}

void CartDlg::OnBnClickedOk()
{
    if (m_vecCart.empty()) { AfxMessageBox(_T("")); return; }
    std::string outID;
    bool ok = OrderManager::GetInstance().ProcessOrder("card_default", 0, "", outID);
    if (!ok) { AfxMessageBox(_T("")); return; }

    DeliveryOkDlg dlg;
    dlg.m_strOrderID = CA2T(outID.c_str(), CP_UTF8);
    this->ShowWindow(SW_HIDE);
    if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
    else this->ShowWindow(SW_SHOW);
}

void CartDlg::OnBnClickedBtnEditOption()
{
    int nIndex = m_listCart.GetSelectionMark();
    if (nIndex == -1) { AfxMessageBox(_T("")); return; }

    CartItem& item = m_vecCart[nIndex];
    OptionChangeDlg dlg;
    dlg.m_strMenuName    = item.menuName.c_str();
    dlg.m_nQuantity      = item.quantity; // CartItem::quantity (O)
    dlg.m_nBasePrice     = item.basePrice;
    if (dlg.DoModal() == IDOK) {
        item.quantity = dlg.m_nQuantity;
        item.CalculateTotalPrice();
        UpdateCartUI();
    }
}

void CartDlg::OnBnClickedBtnBack() { CDialogEx::OnCancel(); }
