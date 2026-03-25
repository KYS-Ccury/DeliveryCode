// MyPageDlg.cpp - My Page screen with JSON protocol
#include "pch.h"
#include "MyPageDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "MyInfoDlg.h"
#include "SubDialogs.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(MyPageDlg, CDialogEx)

BEGIN_MESSAGE_MAP(MyPageDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_TODAY_HISTORY, &MyPageDlg::OnBtnTodayHistory)
    ON_BN_CLICKED(IDC_BTN_SETTLEMENT,    &MyPageDlg::OnBtnSettlement)
    ON_BN_CLICKED(IDC_BTN_DRIVE_TIME,    &MyPageDlg::OnBtnDriveTime)
    ON_BN_CLICKED(IDC_BTN_SETTINGS,      &MyPageDlg::OnBtnSettings)
    ON_BN_CLICKED(IDC_BTN_LOGOUT,        &MyPageDlg::OnBtnLogout)
    ON_BN_CLICKED(IDC_BTN_RIDER_NAME,    &MyPageDlg::OnClickRiderName)
    ON_MESSAGE(WM_SOCKET_RECV,           &MyPageDlg::OnSocketRecv)
    ON_BN_CLICKED(IDCANCEL,             &MyPageDlg::OnBtnBack)
END_MESSAGE_MAP()

MyPageDlg::MyPageDlg(CWnd* pParent) : CDialogEx(IDD_MYPAGE_DLG, pParent) {}
MyPageDlg::~MyPageDlg() {}

void MyPageDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }

BOOL MyPageDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_MY_LIST, GetSafeHwnd());

    const RiderSession& s = AppContext::Get().session;
    CString nameText;
    nameText.Format(_T("%s  >"),
                    static_cast<LPCTSTR>(s.name.IsEmpty() ? s.loginId : s.name));
    SetDlgItemText(IDC_BTN_RIDER_NAME, nameText);
    SetDlgItemText(IDC_STATIC_RIDER_REGION, s.deliveryRegion);
    SetDlgItemText(IDC_STATIC_TODAY_CNT,    _T("0"));
    SetDlgItemText(IDC_STATIC_TODAY_INCOME, _T("0"));
    RequestTodaySummary();
    return TRUE;
}

// Request today's summary: CMD_RIDER_MY_LIST (405), {"summary_only":true}
void MyPageDlg::RequestTodaySummary()
{
    json req; req["summary_only"] = true;
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST, req.dump());
    if (!bSent) {
        SetDlgItemText(IDC_STATIC_TODAY_CNT,    _T("3"));
        SetDlgItemText(IDC_STATIC_TODAY_INCOME, _T("10500"));
    }
}

void MyPageDlg::OnBtnTodayHistory() { TodayHistoryDlg dlg(this); dlg.DoModal(); }
void MyPageDlg::OnBtnSettlement()   { SettlementDlg   dlg(this); dlg.DoModal(); }
void MyPageDlg::OnBtnDriveTime()    { DriveTimeDlg    dlg(this); dlg.DoModal(); }
void MyPageDlg::OnClickRiderName()  { MyInfoDlg       dlg(this); dlg.DoModal(); }
void MyPageDlg::OnBtnSettings()     { SettingsDlg     dlg(this); dlg.DoModal(); }

void MyPageDlg::OnBtnLogout()
{
    if (MessageBox(_T("로그아웃 하시겠습니까?"),
                   _T("로그아웃"), MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    AppContext::Get().socket.SendPacket(CMD_LOGOUT, "{}");
    AppContext::Get().session.isLoggedIn = false;
    AppContext::Get().session.Clear();
    AppContext::Get().currentOrder.Clear();
    EndDialog(IDCANCEL);
}

// Recv 405 summary: {"status":2000,"today_count":3,"today_fee":10500}
LRESULT MyPageDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    if (protocol != CMD_RIDER_MY_LIST) return 0;

    try {
        json res = json::parse(body);
        if (res.value("status", 0) != STATUS_SUCCESS) return 0;
        if (res.contains("today_count")) {
            CString cntStr, incStr;
            cntStr.Format(_T("%d"), res.value("today_count", 0));
            incStr.Format(_T("%d"), res.value("today_fee",   0));
            SetDlgItemText(IDC_STATIC_TODAY_CNT,    cntStr);
            SetDlgItemText(IDC_STATIC_TODAY_INCOME, incStr);
        }
    } catch (...) {}
    return 0;
}

void MyPageDlg::OnBtnBack() { EndDialog(IDCANCEL); }

HBRUSH MyPageDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
