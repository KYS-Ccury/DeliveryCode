// DispatchDlg.cpp - Accept/Reject dispatch with JSON protocol
#include "pch.h"
#include "DispatchDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(DispatchDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DispatchDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_ACCEPT, &DispatchDlg::OnBtnAccept)
    ON_BN_CLICKED(IDC_BTN_REJECT, &DispatchDlg::OnBtnReject)
    ON_WM_TIMER()
END_MESSAGE_MAP()

DispatchDlg::DispatchDlg(const CString& pushData, CWnd* pParent)
    : CDialogEx(IDD_DISPATCH_DLG, pParent), m_pushData(pushData) {}
DispatchDlg::~DispatchDlg() {}

void DispatchDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROGRESS_TIMER, m_progressTimer);
}

BOOL DispatchDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_progressTimer.SetRange(0, 60);
    m_progressTimer.SetPos(60);
    ParsePushData();
    SetDlgItemText(IDC_STATIC_PICKUP, m_pickupAddr);
    SetDlgItemText(IDC_STATIC_DEST,   m_destAddr);
    CString feeStr;
    feeStr.Format(_T("배달료: %d원"), m_deliveryFee);
    SetDlgItemText(IDC_STATIC_FEE, feeStr);
    UpdateTimerUI();
    SetTimer(1, 1000, nullptr);
    return TRUE;
}

// Push format: "700|orderId|storeName|pickupAddr|destAddr|deliveryFee"
void DispatchDlg::ParsePushData()
{
    CString data = m_pushData;
    int p = data.Find(_T('|'));
    if (p >= 0) data = data.Mid(p + 1);

    auto nextField = [&](CString& out) {
        int pipe = data.Find(_T('|'));
        if (pipe >= 0) { out = data.Left(pipe); data = data.Mid(pipe + 1); }
        else           { out = data; data = _T(""); }
    };

    CString tmp;
    nextField(tmp); m_orderId     = _ttoi(tmp);
    nextField(tmp); m_storeName   = tmp;
    nextField(tmp); m_pickupAddr  = tmp;
    nextField(tmp); m_destAddr    = tmp;
    nextField(tmp); m_deliveryFee = _ttoi(tmp);

    CurrentOrder& ord   = AppContext::Get().currentOrder;
    ord.orderId         = m_orderId;
    ord.pickupAddress   = m_pickupAddr;
    ord.deliveryAddress = m_destAddr;
    ord.deliveryFee     = m_deliveryFee;
}

void DispatchDlg::UpdateTimerUI()
{
    CString s; s.Format(_T("배차수락 · %d초"), m_remainSec);
    SetDlgItemText(IDC_STATIC_TIMER, s);
    m_progressTimer.SetPos(m_remainSec);
}

void DispatchDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        if (--m_remainSec <= 0) { KillTimer(1); DoReject(true); }
        else UpdateTimerUI();
    }
    CDialogEx::OnTimer(nIDEvent);
}

// Accept: JSON {"order_id": N}
void DispatchDlg::OnBtnAccept()
{
    KillTimer(1);
    json req; req["order_id"] = m_orderId;
    AppContext::Get().socket.SendPacket(CMD_RIDER_ACCEPT, req.dump());

    if (AppContext::Get().currentOrder.orderCode.IsEmpty()) {
        CString code; code.Format(_T("ORD%06d"), m_orderId);
        AppContext::Get().currentOrder.orderCode = code;
    }
    AppContext::Get().currentOrder.status = _T("PICKUP_MOVING");
    EndDialog(IDOK);
}

void DispatchDlg::OnBtnReject() { KillTimer(1); DoReject(false); }

// Reject: JSON {"order_id": N, "reason": "MANUAL"/"TIMEOUT"}
void DispatchDlg::DoReject(bool bTimeout)
{
    json req;
    req["order_id"] = m_orderId;
    req["reason"]   = bTimeout ? "TIMEOUT" : "MANUAL";
    AppContext::Get().socket.SendPacket(CMD_RIDER_REJECT, req.dump());
    AppContext::Get().currentOrder.Clear();
    EndDialog(IDCANCEL);
}

void DispatchDlg::OnCancel() { KillTimer(1); DoReject(false); }

HBRUSH DispatchDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg)
            m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    // Buttons: do NOT override - let Windows draw button text normally
    return hbr;
}
