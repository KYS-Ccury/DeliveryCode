// PickupCodeDlg.cpp - Order code verification (number pad + QR simulation)
#include "pch.h"
#include "PickupCodeDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(PickupCodeDlg, CDialogEx)

BEGIN_MESSAGE_MAP(PickupCodeDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_NUMBER_PICKUP,  &PickupCodeDlg::OnBtnNumberPickup)
    ON_BN_CLICKED(IDC_BTN_QR_PICKUP,      &PickupCodeDlg::OnBtnQRPickup)
    ON_BN_CLICKED(IDC_BTN_CONFIRM_CODE,   &PickupCodeDlg::OnBtnConfirmCode)
    ON_BN_CLICKED(IDC_KEY_0,  &PickupCodeDlg::OnKey0)
    ON_BN_CLICKED(IDC_KEY_1,  &PickupCodeDlg::OnKey1)
    ON_BN_CLICKED(IDC_KEY_2,  &PickupCodeDlg::OnKey2)
    ON_BN_CLICKED(IDC_KEY_3,  &PickupCodeDlg::OnKey3)
    ON_BN_CLICKED(IDC_KEY_4,  &PickupCodeDlg::OnKey4)
    ON_BN_CLICKED(IDC_KEY_5,  &PickupCodeDlg::OnKey5)
    ON_BN_CLICKED(IDC_KEY_6,  &PickupCodeDlg::OnKey6)
    ON_BN_CLICKED(IDC_KEY_7,  &PickupCodeDlg::OnKey7)
    ON_BN_CLICKED(IDC_KEY_8,  &PickupCodeDlg::OnKey8)
    ON_BN_CLICKED(IDC_KEY_9,  &PickupCodeDlg::OnKey9)
    ON_BN_CLICKED(IDC_KEY_A,  &PickupCodeDlg::OnKeyA)
    ON_BN_CLICKED(IDC_KEY_DEL,&PickupCodeDlg::OnKeyDel)
    ON_WM_TIMER()
END_MESSAGE_MAP()

PickupCodeDlg::PickupCodeDlg(CWnd* pParent)
    : CDialogEx(IDD_PICKUP_CODE_DLG, pParent) {}
PickupCodeDlg::~PickupCodeDlg() {}

void PickupCodeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL PickupCodeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    const CString& code = AppContext::Get().currentOrder.orderCode;
    CString codeDisplay;
    codeDisplay.Format(_T("주문번호: %s"), static_cast<LPCTSTR>(code));
    SetDlgItemText(IDC_STATIC_ORDER_CODE, codeDisplay);

    GetDlgItem(IDC_BTN_CONFIRM_CODE)->EnableWindow(FALSE);
    m_strInput.Empty();
    UpdateCodeDisplay();
    OnBtnNumberPickup();
    return TRUE;
}

void PickupCodeDlg::OnBtnNumberPickup() { ShowQRView(false); }
void PickupCodeDlg::OnBtnQRPickup()     { ShowQRView(true); }

void PickupCodeDlg::ShowQRView(bool bShow)
{
    UINT keyIds[] = { IDC_KEY_0, IDC_KEY_1, IDC_KEY_2, IDC_KEY_3,
                      IDC_KEY_4, IDC_KEY_5, IDC_KEY_6, IDC_KEY_7,
                      IDC_KEY_8, IDC_KEY_9, IDC_KEY_A, IDC_KEY_DEL };
    for (UINT id : keyIds) {
        CWnd* w = GetDlgItem(id);
        if (w) w->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    }
    CWnd* qr = GetDlgItem(IDC_STATIC_QR_VIEW);
    if (qr) {
        qr->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
        if (bShow)
            qr->SetWindowText(_T("[QR 스캔 영역]\n\n배달건 영수증의 QR코드를\n이 영역에 맞춰 스캔해주세요.\n(시뮬레이션: 자동 인식됨)"));
    }
    if (bShow) SetTimer(1, 1000, nullptr);
}

void PickupCodeDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        KillTimer(1);
        // QR pickup: send CMD_RIDER_PICKUP_DONE (403) with JSON
        json req;
        req["order_id"] = AppContext::Get().currentOrder.orderId;
        req["method"]   = "QR";
        AppContext::Get().socket.SendPacket(CMD_RIDER_PICKUP_DONE, req.dump());

        MessageBox(_T("QR 인식 성공!\n픽업이 완료되었습니다."),
                   _T("픽업 완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
    CDialogEx::OnTimer(nIDEvent);
}

void PickupCodeDlg::PressKey(TCHAR ch)
{
    if (m_strInput.GetLength() >= 4) return;
    m_strInput += ch;
    UpdateCodeDisplay();
    if (m_strInput.GetLength() == 4)
        GetDlgItem(IDC_BTN_CONFIRM_CODE)->EnableWindow(TRUE);
}

void PickupCodeDlg::UpdateCodeDisplay()
{
    UINT boxIds[] = { IDC_EDIT_CODE_1, IDC_EDIT_CODE_2,
                      IDC_EDIT_CODE_3, IDC_EDIT_CODE_4 };
    for (int i = 0; i < 4; i++) {
        CWnd* w = GetDlgItem(boxIds[i]);
        if (!w) continue;
        w->SetWindowText(i < m_strInput.GetLength()
                         ? CString(m_strInput[i]) : _T(""));
    }
}

void PickupCodeDlg::OnBtnConfirmCode()
{
    const CString& fullCode = AppContext::Get().currentOrder.orderCode;
    CString expected = (fullCode.GetLength() >= 4)
                       ? fullCode.Right(4) : fullCode;

    if (m_strInput.CompareNoCase(expected) != 0) {
        MessageBox(_T("주문번호가 올바르지 않습니다.\n영수증을 다시 확인해주세요."),
                   _T("오류"), MB_OK | MB_ICONWARNING);
        m_strInput.Empty();
        UpdateCodeDisplay();
        GetDlgItem(IDC_BTN_CONFIRM_CODE)->EnableWindow(FALSE);
        return;
    }

    // Number pickup: send CMD_RIDER_PICKUP_DONE (403) with JSON
    json req;
    req["order_id"] = AppContext::Get().currentOrder.orderId;
    req["method"]   = "NUMBER";
    AppContext::Get().socket.SendPacket(CMD_RIDER_PICKUP_DONE, req.dump());

    MessageBox(_T("픽업이 완료되었습니다!"), _T("완료"), MB_OK | MB_ICONINFORMATION);
    EndDialog(IDOK);
}

void PickupCodeDlg::OnKeyDel()
{
    if (!m_strInput.IsEmpty()) {
        m_strInput = m_strInput.Left(m_strInput.GetLength() - 1);
        UpdateCodeDisplay();
        GetDlgItem(IDC_BTN_CONFIRM_CODE)->EnableWindow(FALSE);
    }
}

void PickupCodeDlg::OnKey0() { PressKey(_T('0')); }
void PickupCodeDlg::OnKey1() { PressKey(_T('1')); }
void PickupCodeDlg::OnKey2() { PressKey(_T('2')); }
void PickupCodeDlg::OnKey3() { PressKey(_T('3')); }
void PickupCodeDlg::OnKey4() { PressKey(_T('4')); }
void PickupCodeDlg::OnKey5() { PressKey(_T('5')); }
void PickupCodeDlg::OnKey6() { PressKey(_T('6')); }
void PickupCodeDlg::OnKey7() { PressKey(_T('7')); }
void PickupCodeDlg::OnKey8() { PressKey(_T('8')); }
void PickupCodeDlg::OnKey9() { PressKey(_T('9')); }
void PickupCodeDlg::OnKeyA() { PressKey(_T('A')); }

HBRUSH PickupCodeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg) m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    return hbr;
}
