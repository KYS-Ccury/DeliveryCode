#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OrderHistoryDlg.h"
#include "ChatDlg.h"

IMPLEMENT_DYNAMIC(OrderHistoryDlg, CDialogEx)

OrderHistoryDlg::OrderHistoryDlg(CWnd* pParent)
    : CDialogEx(IDD_ORDERHISTORY_DLG, pParent) {}
OrderHistoryDlg::~OrderHistoryDlg() {}

void OrderHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(OrderHistoryDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK, &OrderHistoryDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_CHAT, &OrderHistoryDlg::OnBnClickedBtnChat)
END_MESSAGE_MAP()

BOOL OrderHistoryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetDlgItemText(IDC_STATIC_ORDER_NUM,
        _T(": ") + m_strOrderNum);
    SetDlgItemText(IDC_STATIC_SHOP_NAME,
        _T(": ") + m_strShopName);
    CString s; s.Format(_T(": %d"), m_nTotalAmount);
    SetDlgItemText(IDC_STATIC_FINAL_TOTAL, s);
    return TRUE;
}

void OrderHistoryDlg::OnBnClickedBtnBack() { OnCancel(); }

void OrderHistoryDlg::OnBnClickedBtnChat()
{
    ChatDlg dlg;
    dlg.m_strTargetName = _T("");
    dlg.m_strTargetID   = _T("admin");
    dlg.m_strTargetType = _T("admin");
    dlg.DoModal();
}
