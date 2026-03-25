// MyInfoDlg.cpp - MyInfoDlg / ChangePwDlg / ChangeAcctDlg (NO MyPageDlg here)
#include "pch.h"
#include "MyInfoDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "SubDialogs.h"
#include "json.hpp"
using json = nlohmann::json;

// ── 팝업 창을 부모 창 안에 중앙 정렬하는 헬퍼 ─────────────
static void CenterInParent(CWnd* pDlg, CWnd* pParent)
{
    if (!pParent || !pDlg) return;
    CRect rcParent, rcDlg;
    pParent->GetWindowRect(&rcParent);
    pDlg->GetWindowRect(&rcDlg);

    int dlgW = rcDlg.Width();
    int dlgH = rcDlg.Height();

    // 부모 중앙 계산
    int x = rcParent.left + (rcParent.Width()  - dlgW) / 2;
    int y = rcParent.top  + (rcParent.Height() - dlgH) / 2;

    // 부모 영역 밖으로 나가지 않도록 클램프
    x = max(rcParent.left, min(x, rcParent.right  - dlgW));
    y = max(rcParent.top,  min(y, rcParent.bottom - dlgH));

    pDlg->SetWindowPos(nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// ================================================================
//  MyInfoDlg
// ================================================================
IMPLEMENT_DYNAMIC(MyInfoDlg, CDialogEx)

BEGIN_MESSAGE_MAP(MyInfoDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_VEHICLE,     &MyInfoDlg::OnBtnVehicle)
    ON_BN_CLICKED(IDC_BTN_CHANGE_PW,   &MyInfoDlg::OnBtnChangePw)
    ON_BN_CLICKED(IDC_BTN_CHANGE_ACCT, &MyInfoDlg::OnBtnChangeAcct)
    ON_MESSAGE(WM_SOCKET_RECV,         &MyInfoDlg::OnSocketRecv)
END_MESSAGE_MAP()

MyInfoDlg::MyInfoDlg(CWnd* pParent) : CDialogEx(IDD_MYINFO_DLG, pParent) {}
MyInfoDlg::~MyInfoDlg() {}

void MyInfoDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }

BOOL MyInfoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.RegisterWnd(CMD_GET_MY_INFO, GetSafeHwnd());
    CenterInParent(this, GetParent());

    const RiderSession& s = AppContext::Get().session;
    SetDlgItemText(IDC_STATIC_LOGIN_ID, s.loginId);

    CString vehicleText;
    vehicleText.Format(_T("%s  >"),
        static_cast<LPCTSTR>(s.vehicleType.IsEmpty() ? CString(_T("-")) : s.vehicleType));
    SetDlgItemText(IDC_BTN_VEHICLE, vehicleText);
    SetDlgItemText(IDC_STATIC_REGION, s.deliveryRegion);
    return TRUE;
}

// Vehicle change: CMD_RIDER_STATUS_UPDATE(406), {"action":"VEHICLE","vehicle_type":"..."}
void MyInfoDlg::OnBtnVehicle()
{
    static const LPCTSTR vehicles[] = {
        _T("도보"), _T("자전거"), _T("오토바이"),
        _T("전기자전거(PAS)"), _T("전기자전거(스로틀)"),
        _T("킥보드"), _T("자동차")
    };
    const int CNT = 7;

    CMenu menu; menu.CreatePopupMenu();
    for (int i = 0; i < CNT; i++)
        menu.AppendMenu(MF_STRING, 2000 + i, vehicles[i]);

    CWnd* pBtn = GetDlgItem(IDC_BTN_VEHICLE);
    CRect rc; pBtn->GetWindowRect(&rc);
    int sel = menu.TrackPopupMenu(
        TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        rc.left, rc.bottom, this);

    if (sel >= 2000 && sel < 2000 + CNT) {
        CString chosen = vehicles[sel - 2000];
        AppContext::Get().session.vehicleType = chosen;

        CT2A chosenUtf8(chosen, CP_UTF8);
        json req;
        req["action"]       = "VEHICLE";
        req["vehicle_type"] = std::string(chosenUtf8);
        AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, req.dump());

        CString btnText; btnText.Format(_T("%s  >"), static_cast<LPCTSTR>(chosen));
        SetDlgItemText(IDC_BTN_VEHICLE, btnText);
    }
}

void MyInfoDlg::OnBtnChangePw()   { ChangePwDlg   dlg(this); dlg.DoModal(); }
void MyInfoDlg::OnBtnChangeAcct() { ChangeAcctDlg dlg(this); dlg.DoModal(); }

LRESULT MyInfoDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (pPkt) delete pPkt;
    return 0;
}

HBRUSH MyInfoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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

// ================================================================
//  ChangePwDlg
// ================================================================
IMPLEMENT_DYNAMIC(ChangePwDlg, CDialogEx)

BEGIN_MESSAGE_MAP(ChangePwDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_PW_CONFIRM, &ChangePwDlg::OnBtnConfirm)
    ON_MESSAGE(WM_SOCKET_RECV,        &ChangePwDlg::OnSocketRecv)
END_MESSAGE_MAP()

ChangePwDlg::ChangePwDlg(CWnd* pParent) : CDialogEx(IDD_CHANGE_PW_DLG, pParent) {}
ChangePwDlg::~ChangePwDlg() {}

void ChangePwDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_CUR_PW,         m_editCurPw);
    DDX_Control(pDX, IDC_EDIT_NEW_PW,         m_editNewPw);
    DDX_Control(pDX, IDC_EDIT_NEW_PW_CONFIRM, m_editNewPwConfirm);
}

BOOL ChangePwDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.RegisterWnd(CMD_GET_MY_INFO, GetSafeHwnd());
    CenterInParent(this, GetParent());
    return TRUE;
}

// Send: CMD_GET_MY_INFO(104), {"action":"CHANGE_PW","cur_pw":"...","new_pw":"..."}
void ChangePwDlg::OnBtnConfirm()
{
    CString curPw, newPw, newPwConfirm;
    GetDlgItemText(IDC_EDIT_CUR_PW,         curPw);
    GetDlgItemText(IDC_EDIT_NEW_PW,         newPw);
    GetDlgItemText(IDC_EDIT_NEW_PW_CONFIRM, newPwConfirm);
    curPw.Trim(); newPw.Trim(); newPwConfirm.Trim();

    if (curPw.IsEmpty()) {
        MessageBox(_T("현재 비밀번호를 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (newPw.GetLength() < 10) {
        MessageBox(_T("새 비밀번호는 영문, 숫자 혼합 10자리 이상이어야 합니다."),
                   _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (newPw != newPwConfirm) {
        MessageBox(_T("비밀번호가 일치하지 않습니다."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }

    auto toU = [](const CString& s) -> std::string {
        CT2A u(s, CP_UTF8); return std::string(u);
    };
    json req;
    req["action"] = "CHANGE_PW";
    req["cur_pw"] = toU(curPw);
    req["new_pw"] = toU(newPw);

    bool bSent = AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, req.dump());
    if (!bSent) {
        MessageBox(_T("비밀번호가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
}

// Recv: {"status":2000,"action":"CHANGE_PW"} or {"status":4001,"message":"..."}
LRESULT ChangePwDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    if (protocol != CMD_GET_MY_INFO) return 0;

    try {
        json res = json::parse(body);
        if (!res.contains("action") || res["action"] != "CHANGE_PW") return 0;
        if (res.value("status", 0) == STATUS_SUCCESS) {
            MessageBox(_T("비밀번호가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
            EndDialog(IDOK);
        } else {
            std::string msg = res.value("message", "Current password is incorrect.");
            CA2T wMsg(msg.c_str(), CP_UTF8);
            MessageBox(CString(wMsg), _T("실패"), MB_OK | MB_ICONWARNING);
        }
    } catch (...) {}
    return 0;
}

HBRUSH ChangePwDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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

// ================================================================
//  ChangeAcctDlg
// ================================================================
IMPLEMENT_DYNAMIC(ChangeAcctDlg, CDialogEx)

BEGIN_MESSAGE_MAP(ChangeAcctDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_ACCT_CHANGE, &ChangeAcctDlg::OnBtnChange)
    ON_MESSAGE(WM_SOCKET_RECV,         &ChangeAcctDlg::OnSocketRecv)
END_MESSAGE_MAP()

ChangeAcctDlg::ChangeAcctDlg(CWnd* pParent)
    : CDialogEx(IDD_CHANGE_ACCT_DLG, pParent), m_bEditMode(false) {}
ChangeAcctDlg::~ChangeAcctDlg() {}

void ChangeAcctDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_BANK,    m_editBank);
    DDX_Control(pDX, IDC_EDIT_HOLDER,  m_editHolder);
    DDX_Control(pDX, IDC_EDIT_ACCOUNT, m_editAccount);
}

BOOL ChangeAcctDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.RegisterWnd(CMD_GET_MY_INFO, GetSafeHwnd());
    CenterInParent(this, GetParent());

    const RiderSession& s = AppContext::Get().session;

    // 세션에 계좌 정보가 있으면 바로 표시, 없으면 서버에서 조회
    if (!s.bankName.IsEmpty() || !s.accountHolder.IsEmpty()) {
        SetDlgItemText(IDC_STATIC_BANK,   s.bankName);
        SetDlgItemText(IDC_STATIC_HOLDER, s.accountHolder);
        CString masked;
        if (s.accountNumber.GetLength() > 4) {
            CString stars(_T('*'), s.accountNumber.GetLength() - 4);
            masked = stars + s.accountNumber.Right(4);
        } else {
            masked = s.accountNumber;
        }
        SetDlgItemText(IDC_STATIC_ACCOUNT, masked);
    } else {
        // 서버에서 프로필 조회 (계좌 정보 포함) - "{}" 빈 오브젝트 전송
        AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, "{}");
        SetDlgItemText(IDC_STATIC_BANK,    _T("-"));
        SetDlgItemText(IDC_STATIC_HOLDER,  _T("-"));
        SetDlgItemText(IDC_STATIC_ACCOUNT, _T("조회 중..."));
    }
    ShowEditMode(false);
    return TRUE;
}

void ChangeAcctDlg::OnBtnChange()
{
    if (!m_bEditMode) ShowEditMode(true);
    else DoSaveAcct();
}

void ChangeAcctDlg::ShowEditMode(bool bEdit)
{
    m_bEditMode = bEdit;
    auto showCtrl = [this](UINT id, BOOL bShow) {
        CWnd* w = GetDlgItem(id);
        if (w) w->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
    };
    showCtrl(IDC_STATIC_BANK,    !bEdit);
    showCtrl(IDC_STATIC_HOLDER,  !bEdit);
    showCtrl(IDC_STATIC_ACCOUNT, !bEdit);
    showCtrl(IDC_EDIT_BANK,       bEdit);
    showCtrl(IDC_EDIT_HOLDER,     bEdit);
    showCtrl(IDC_EDIT_ACCOUNT,    bEdit);

    if (bEdit) {
        const RiderSession& s = AppContext::Get().session;
        SetDlgItemText(IDC_EDIT_BANK,    s.bankName);
        SetDlgItemText(IDC_EDIT_HOLDER,  s.accountHolder);
        SetDlgItemText(IDC_EDIT_ACCOUNT, s.accountNumber);
        SetDlgItemText(IDC_BTN_ACCT_CHANGE, _T("저장하기"));
    } else {
        SetDlgItemText(IDC_BTN_ACCT_CHANGE, _T("변경하기"));
    }
}

// Send: CMD_GET_MY_INFO(104), {"action":"CHANGE_ACCT","bank","holder","account"}
void ChangeAcctDlg::DoSaveAcct()
{
    CString bank, holder, account;
    GetDlgItemText(IDC_EDIT_BANK,    bank);
    GetDlgItemText(IDC_EDIT_HOLDER,  holder);
    GetDlgItemText(IDC_EDIT_ACCOUNT, account);
    bank.Trim(); holder.Trim(); account.Trim();

    if (bank.IsEmpty()) {
        MessageBox(_T("은행명을 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING); return;
    }
    if (holder.IsEmpty()) {
        MessageBox(_T("예금주를 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING); return;
    }
    if (account.GetLength() < 10) {
        MessageBox(_T("계좌번호를 정확히 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING); return;
    }

    AppContext::Get().session.bankName      = bank;
    AppContext::Get().session.accountHolder = holder;
    AppContext::Get().session.accountNumber = account;

    // Remove whitespace/tab chars before JSON encoding
    bank.Replace(_T("\t"), _T(""));
    holder.Replace(_T("\t"), _T(""));
    account.Replace(_T("\t"), _T(""));
    bank.Trim(); holder.Trim(); account.Trim();

    auto toU = [](const CString& s) -> std::string {
        CT2A u(s, CP_UTF8); return std::string(u);
    };
    json req;
    req["action"]  = "CHANGE_ACCT";
    req["bank"]    = toU(bank);
    req["holder"]  = toU(holder);
    req["account"] = toU(account);

    bool bSent = AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, req.dump());
    if (!bSent) {
        MessageBox(_T("계좌 정보가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        ShowEditMode(false);
        SetDlgItemText(IDC_STATIC_BANK,   bank);
        SetDlgItemText(IDC_STATIC_HOLDER, holder);
        CString masked(_T('*'), account.GetLength() - 4);
        masked += account.Right(4);
        SetDlgItemText(IDC_STATIC_ACCOUNT, masked);
    }
}

// Recv: {"status":2000,"action":"CHANGE_ACCT"} 또는 프로필 조회 응답
LRESULT ChangeAcctDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    if (protocol != CMD_GET_MY_INFO) return 0;

    try {
        json res = json::parse(body);
        if (res.value("status", 0) != STATUS_SUCCESS) return 0;

        std::string action = res.value("action", "");

        // 계좌 변경 완료 응답
        if (action == "CHANGE_ACCT") {
            MessageBox(_T("계좌 정보가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
            ShowEditMode(false);
            return 0;
        }

        // 프로필 조회 응답 (bank_info, acct_masked 필드가 있으면 계좌 정보 표시)
        if (res.contains("bank_info")) {
            auto toCS = [](const std::string& s) -> CString {
                CA2T ws(s.c_str(), CP_UTF8); return CString(ws);
            };
            CString bankInfo  = toCS(res.value("bank_info",  ""));
            CString acctMask  = toCS(res.value("acct_masked", ""));

            // bank_info = "은행명 예금주" 형식으로 저장됨
            int sp = bankInfo.Find(_T(' '));
            if (sp > 0) {
                AppContext::Get().session.bankName      = bankInfo.Left(sp);
                AppContext::Get().session.accountHolder = bankInfo.Mid(sp + 1);
            } else {
                AppContext::Get().session.bankName = bankInfo;
            }
            AppContext::Get().session.accountNumber = acctMask;

            SetDlgItemText(IDC_STATIC_BANK,    AppContext::Get().session.bankName);
            SetDlgItemText(IDC_STATIC_HOLDER,  AppContext::Get().session.accountHolder);
            SetDlgItemText(IDC_STATIC_ACCOUNT, acctMask.IsEmpty() ? CString(_T("-")) : acctMask);
        }
    } catch (...) {}
    return 0;
}

HBRUSH ChangeAcctDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
