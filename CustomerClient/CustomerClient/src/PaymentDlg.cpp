#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "PaymentDlg.h"

IMPLEMENT_DYNAMIC(PaymentDlg, CDialogEx)

PaymentDlg::PaymentDlg(CWnd* pParent)
    : CDialogEx(IDD_PAYMENT_DLG, pParent) {}
PaymentDlg::~PaymentDlg() {}

void PaymentDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(PaymentDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,     &PaymentDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &PaymentDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

BOOL PaymentDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    return TRUE;
}

void PaymentDlg::OnBnClickedOk()
{
    CDialogEx::OnOK();
}

void PaymentDlg::OnBnClickedCancel()
{
    CDialogEx::OnCancel();
}
