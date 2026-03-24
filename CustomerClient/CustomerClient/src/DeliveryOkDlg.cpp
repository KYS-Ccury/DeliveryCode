#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "DeliveryOkDlg.h"
#include "ReviewWriteDlg.h"
#include "OrderManager.h"

IMPLEMENT_DYNAMIC(DeliveryOkDlg, CDialogEx)

DeliveryOkDlg::DeliveryOkDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_OK_DLG, pParent) {}
DeliveryOkDlg::~DeliveryOkDlg() {}

void DeliveryOkDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(DeliveryOkDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_WRITE_REVIEW, &DeliveryOkDlg::OnBnClickedBtnWriteReview)
    ON_MESSAGE(WM_ORDER_STATUS_CHANGED, &DeliveryOkDlg::OnOrderStatusChanged)
END_MESSAGE_MAP()

BOOL DeliveryOkDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetDlgItemText(IDC_STATIC_STORE_NAME, m_strStoreName);
    SetDlgItemText(IDC_STATIC_ORDER_LIST, m_strOrderList);
    CString s; s.Format(_T("%d"), m_nTotalAmount);
    SetDlgItemText(IDC_STATIC_PRICE, s);
    UpdateStatusUI(STATUS_WAITING); // DeliveryStatus::STATUS_WAITING
    GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(FALSE);

    HWND hThis = GetSafeHwnd();
    OrderManager::GetInstance().RegisterOrderStatusCallback(
        [hThis](const std::string& orderID, int status, const std::string& msg) {
            int* p = new int(status);
            ::PostMessage(hThis, WM_ORDER_STATUS_CHANGED, (WPARAM)p, 0);
        }
    );
    return TRUE;
}

LRESULT DeliveryOkDlg::OnOrderStatusChanged(WPARAM wParam, LPARAM lParam)
{
    int* p = reinterpret_cast<int*>(wParam);
    int status = p ? *p : 0;
    delete p;
    UpdateStatusUI(status);
    if (status == STATUS_COMPLETE) { // DeliveryStatus::STATUS_COMPLETE
        GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(TRUE);
        AfxMessageBox(_T(""));
    }
    return 0;
}

void DeliveryOkDlg::UpdateStatusUI(int status)
{
    // DeliveryStatus: STATUS_WAITING=0, STATUS_PREPARING=1,
    //                 STATUS_DELIVERING=2, STATUS_COMPLETE=3
    const TCHAR* labels[] = { _T(""), _T(""), _T(""), _T("") };
    CString s;
    for (int i = 0; i < 4; ++i) {
        if      (i < status)  s += CString(_T("V ")) + labels[i] + _T("  ");
        else if (i == status) s += CString(_T("> ")) + labels[i] + _T("  ");
        else                  s += CString(_T("O ")) + labels[i] + _T("  ");
    }
    SetDlgItemText(IDC_STATIC_TIME, s);
}

void DeliveryOkDlg::OnBnClickedBtnWriteReview()
{
    ReviewWriteDlg dlg;
    if (dlg.DoModal() == IDOK) {
        AfxMessageBox(_T(""));
        GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(FALSE);
    }
}
