#include "pch.h"
#include "MyPageDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "MyInfoDlg.h"
#include "SubDialogs.h"

IMPLEMENT_DYNAMIC(MyPageDlg, CDialogEx)

BEGIN_MESSAGE_MAP(MyPageDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_TODAY_HISTORY, &MyPageDlg::OnBtnTodayHistory)
    ON_BN_CLICKED(IDC_BTN_SETTLEMENT,    &MyPageDlg::OnBtnSettlement)
    ON_BN_CLICKED(IDC_BTN_DRIVE_TIME,    &MyPageDlg::OnBtnDriveTime)
    ON_BN_CLICKED(IDC_BTN_MY_INFO,       &MyPageDlg::OnBtnMyInfo)
    ON_BN_CLICKED(IDC_BTN_SETTINGS,      &MyPageDlg::OnBtnSettings)
    ON_BN_CLICKED(IDC_BTN_LOGOUT,        &MyPageDlg::OnBtnLogout)
    ON_MESSAGE(WM_SOCKET_RECV,            &MyPageDlg::OnSocketRecv)
END_MESSAGE_MAP()

MyPageDlg::MyPageDlg(CWnd* pParent) : CDialogEx(IDD_MYPAGE_DLG, pParent) {}
MyPageDlg::~MyPageDlg() {}

BOOL MyPageDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    const RiderSession& s = AppContext::Get().session;

    // 라이더 이름 표시: "홍길동 라이더님"
    CString nameText;
    nameText.Format(_T("%s 라이더님  >"),
                    static_cast<LPCTSTR>(s.name.IsEmpty() ? s.loginId : s.name));
    SetDlgItemText(IDC_STATIC_RIDER_NAME, nameText);

    // 배달지역 표시
    SetDlgItemText(IDC_STATIC_RIDER_REGION, s.deliveryRegion);

    // 오늘 요약 초기화 후 서버 요청
    SetDlgItemText(IDC_STATIC_TODAY_CNT,    _T("0건"));
    SetDlgItemText(IDC_STATIC_TODAY_INCOME, _T("0원"));
    RequestTodaySummary();

    return TRUE;
}

// ─────────────────────────────────────────────
// 서버에 오늘 요약 요청
// ─────────────────────────────────────────────
void MyPageDlg::RequestTodaySummary()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST, _T("SUMMARY"));
    if (!bSent) {
        // 서버 미연결 시 더미 데이터
        SetDlgItemText(IDC_STATIC_TODAY_CNT,    _T("3건"));
        SetDlgItemText(IDC_STATIC_TODAY_INCOME, _T("10,500원"));
    }
}

// ─────────────────────────────────────────────
// 버튼 핸들러
// ─────────────────────────────────────────────
void MyPageDlg::OnBtnTodayHistory()
{
    TodayHistoryDlg dlg(this);
    dlg.DoModal();
}

void MyPageDlg::OnBtnSettlement()
{
    SettlementDlg dlg(this);
    dlg.DoModal();
}

void MyPageDlg::OnBtnDriveTime()
{
    DriveTimeDlg dlg(this);
    dlg.DoModal();
}

void MyPageDlg::OnBtnMyInfo()
{
    MyInfoDlg dlg(this);
    dlg.DoModal();
}

void MyPageDlg::OnBtnSettings()
{
    SettingsDlg dlg(this);
    dlg.DoModal();
}

void MyPageDlg::OnBtnLogout()
{
    if (MessageBox(_T("로그아웃 하시겠습니까?"),
                   _T("로그아웃"), MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    AppContext::Get().socket.SendPacket(CMD_LOGOUT);
    AppContext::Get().session.isLoggedIn = false;
    AppContext::Get().session.Clear();
    AppContext::Get().currentOrder.Clear();

    EndDialog(IDCANCEL);  // MainDlg에 로그아웃 신호
}

// ─────────────────────────────────────────────
// 서버 응답: "405|OK|count\tincome"
// ─────────────────────────────────────────────
LRESULT MyPageDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    int cmd = _ttoi(msg.Left(p));
    if (cmd != CMD_RIDER_MY_LIST) return 0;

    CString rest = msg.Mid(p + 1);
    int ok = rest.Find(_T('|'));
    if (ok < 0) return 0;
    if (rest.Left(ok) != _T("OK")) return 0;

    ParseTodaySummary(rest.Mid(ok + 1));
    return 0;
}

// payload: "count\tincome"
void MyPageDlg::ParseTodaySummary(const CString& payload)
{
    int tab = payload.Find(_T('\t'));
    if (tab < 0) return;

    int cnt    = _ttoi(payload.Left(tab));
    int income = _ttoi(payload.Mid(tab + 1));

    CString cntStr, incomeStr;
    cntStr.Format(_T("%d건"), cnt);
    incomeStr.Format(_T("%d원"), income);

    SetDlgItemText(IDC_STATIC_TODAY_CNT,    cntStr);
    SetDlgItemText(IDC_STATIC_TODAY_INCOME, incomeStr);
}

void MyPageDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}
