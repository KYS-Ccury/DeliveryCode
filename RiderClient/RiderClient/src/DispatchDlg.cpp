#include "pch.h"
#include "DispatchDlg.h"
#include "Protocol.h"
#include "AppContext.h"

IMPLEMENT_DYNAMIC(DispatchDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DispatchDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_ACCEPT, &DispatchDlg::OnBtnAccept)
    ON_BN_CLICKED(IDC_BTN_REJECT, &DispatchDlg::OnBtnReject)
    ON_WM_TIMER()
END_MESSAGE_MAP()

DispatchDlg::DispatchDlg(const CString& pushData, CWnd* pParent)
    : CDialogEx(IDD_DISPATCH_DLG, pParent)
    , m_pushData(pushData)
{
}

DispatchDlg::~DispatchDlg()
{
}

void DispatchDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROGRESS_TIMER, m_progressTimer);
}

BOOL DispatchDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 진행바 범위 설정 (30 → 0)
    m_progressTimer.SetRange(0, 30);
    m_progressTimer.SetPos(30);

    // 푸시 데이터 파싱 후 컨트롤 채우기
    ParsePushData();

    SetDlgItemText(IDC_STATIC_PICKUP, m_pickupAddr);
    SetDlgItemText(IDC_STATIC_DEST,   m_destAddr);

    CString feeStr;
    feeStr.Format(_T("배달료  %d원"), m_deliveryFee);
    SetDlgItemText(IDC_STATIC_FEE, feeStr);

    UpdateTimerUI();

    // 1초 타이머 시작
    SetTimer(1, 1000, nullptr);

    return TRUE;
}

// ─────────────────────────────────────────────
// 푸시 데이터 파싱
// 형식: "700|orderId|storeName|pickupAddr|destAddr|deliveryFee"
// ─────────────────────────────────────────────
void DispatchDlg::ParsePushData()
{
    // 앞의 "700|" 제거
    CString data = m_pushData;
    int p = data.Find(_T('|'));
    if (p >= 0) data = data.Mid(p + 1);  // orderId|...

    auto nextField = [&](CString& out) {
        int pipe = data.Find(_T('|'));
        if (pipe >= 0) {
            out  = data.Left(pipe);
            data = data.Mid(pipe + 1);
        } else {
            out  = data;
            data = _T("");
        }
    };

    CString tmp;
    nextField(tmp);  m_orderId     = _ttoi(tmp);
    nextField(tmp);  m_storeName   = tmp;
    nextField(tmp);  m_pickupAddr  = tmp;
    nextField(tmp);  m_destAddr    = tmp;
    nextField(tmp);  m_deliveryFee = _ttoi(tmp);

    // Fill currentOrder in advance
    CurrentOrder& ord    = AppContext::Get().currentOrder;
    ord.orderId          = m_orderId;
    ord.pickupAddress    = m_pickupAddr;
    ord.deliveryAddress  = m_destAddr;
    ord.deliveryFee      = m_deliveryFee;
}

// ─────────────────────────────────────────────
// 타이머 UI 갱신
// ─────────────────────────────────────────────
void DispatchDlg::UpdateTimerUI()
{
    CString timerStr;
    timerStr.Format(_T("배차수락 · %d초"), m_remainSec);
    SetDlgItemText(IDC_STATIC_TIMER, timerStr);

    m_progressTimer.SetPos(m_remainSec);
}

// ─────────────────────────────────────────────
// 1초 타이머
// ─────────────────────────────────────────────
void DispatchDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        --m_remainSec;
        UpdateTimerUI();
        if (m_remainSec <= 0) {
            KillTimer(1);
            DoReject(true);  // 시간 초과 → 자동 거절
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

// ─────────────────────────────────────────────
// 수락 버튼
// ─────────────────────────────────────────────
void DispatchDlg::OnBtnAccept()
{
    KillTimer(1);

    CString payload;
    payload.Format(_T("%d"), m_orderId);
    AppContext::Get().socket.SendPacket(CMD_RIDER_ACCEPT, payload);

    // currentOrder의 orderCode는 서버 응답에서 채워지거나
    // 서버 미연결 시 임시값 사용
    if (AppContext::Get().currentOrder.orderCode.IsEmpty()) {
        CString code;
        code.Format(_T("ORD%04d"), m_orderId);
        AppContext::Get().currentOrder.orderCode = code;
    }
    AppContext::Get().currentOrder.status = _T("PICKUP_MOVING");

    EndDialog(IDOK);
}

// ─────────────────────────────────────────────
// 거절 버튼
// ─────────────────────────────────────────────
void DispatchDlg::OnBtnReject()
{
    KillTimer(1);
    DoReject(false);
}

// ─────────────────────────────────────────────
// 거절 공통 처리
// ─────────────────────────────────────────────
void DispatchDlg::DoReject(bool bTimeout)
{
    CString payload;
    payload.Format(_T("%d|%s"), m_orderId, bTimeout ? _T("TIMEOUT") : _T("MANUAL"));
    AppContext::Get().socket.SendPacket(CMD_RIDER_REJECT, payload);

    AppContext::Get().currentOrder.Clear();
    EndDialog(IDCANCEL);
}

void DispatchDlg::OnCancel()
{
    KillTimer(1);
    DoReject(false);
}
