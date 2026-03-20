#include "pch.h"
#include "SubDialogs.h"
#include "Protocol.h"
#include "AppContext.h"

// ══════════════════════════════════════════════════════════════
//  SettingsDlg
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(SettingsDlg, CDialogEx)

BEGIN_MESSAGE_MAP(SettingsDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REGION_NEWS,   &SettingsDlg::OnBtnRegionNews)
    ON_BN_CLICKED(IDC_BTN_DISPATCH_TYPE, &SettingsDlg::OnBtnDispatchType)
    ON_BN_CLICKED(IDOK,                  &SettingsDlg::OnBtnSave)
END_MESSAGE_MAP()

SettingsDlg::SettingsDlg(CWnd* pParent) : CDialogEx(IDD_SETTINGS_DLG, pParent) {}
SettingsDlg::~SettingsDlg() {}

BOOL SettingsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    LoadSettings();

    // 소식받기 지역 에디트 초기값 설정
    SetDlgItemText(IDC_EDIT_REGION_NEWS, m_strRegionNews);

    // 배차 방식 Static 표시
    SetDlgItemText(IDC_STATIC_DISPATCH_TYPE, m_strDispatchType);

    return TRUE;
}

// 소식받기 지역 변경
void SettingsDlg::OnBtnRegionNews()
{
    CString strNew;
    GetDlgItemText(IDC_EDIT_REGION_NEWS, strNew);
    strNew.Trim();
    if (strNew.IsEmpty()) {
        MessageBox(_T("지역을 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    m_strRegionNews = strNew;
    MessageBox(_T("소식받기 지역이 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
}

// 배차 방식 선택 팝업
void SettingsDlg::OnBtnDispatchType()
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1001, _T("자동배차 (가까운 주문 자동 수락)"));
    menu.AppendMenu(MF_STRING, 1002, _T("수동배차 (요청을 직접 수락/거절)"));

    CWnd* pBtn = GetDlgItem(IDC_BTN_DISPATCH_TYPE);
    CRect rc;
    pBtn->GetWindowRect(&rc);

    int sel = menu.TrackPopupMenu(
        TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        rc.left, rc.bottom, this);

    if (sel == 1001) {
        m_strDispatchType = _T("자동배차");
    } else if (sel == 1002) {
        m_strDispatchType = _T("수동배차");
    }
    if (sel != 0)
        SetDlgItemText(IDC_STATIC_DISPATCH_TYPE, m_strDispatchType);
}

void SettingsDlg::OnBtnSave()
{
    SaveSettings();
    EndDialog(IDOK);
}

void SettingsDlg::SaveSettings()
{
    HKEY hKey = nullptr;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\BaeminRider\\Settings"),
                   0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (!hKey) return;

    auto writeStr = [&](LPCTSTR name, const CString& val) {
        RegSetValueEx(hKey, name, 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(static_cast<LPCTSTR>(val)),
                      (val.GetLength() + 1) * sizeof(TCHAR));
    };
    writeStr(_T("RegionNews"),   m_strRegionNews);
    writeStr(_T("DispatchType"), m_strDispatchType);
    RegCloseKey(hKey);
}

void SettingsDlg::LoadSettings()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\BaeminRider\\Settings"),
                     0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        // 기본값
        m_strRegionNews   = AppContext::Get().session.deliveryRegion;
        m_strDispatchType = _T("수동배차");
        return;
    }
    auto readStr = [&](LPCTSTR name, CString& out, LPCTSTR def) {
        TCHAR buf[256] = {};
        DWORD size = sizeof(buf), type = REG_SZ;
        if (RegQueryValueEx(hKey, name, nullptr, &type,
                            reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS)
            out = buf;
        else
            out = def;
    };
    readStr(_T("RegionNews"),   m_strRegionNews,
            static_cast<LPCTSTR>(AppContext::Get().session.deliveryRegion));
    readStr(_T("DispatchType"), m_strDispatchType, _T("수동배차"));
    RegCloseKey(hKey);
}


// ══════════════════════════════════════════════════════════════
//  SettlementDlg
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(SettlementDlg, CDialogEx)

BEGIN_MESSAGE_MAP(SettlementDlg, CDialogEx)
    ON_MESSAGE(WM_SOCKET_RECV, &SettlementDlg::OnSocketRecv)
END_MESSAGE_MAP()

SettlementDlg::SettlementDlg(CWnd* pParent) : CDialogEx(IDD_SETTLEMENT_DLG, pParent) {}
SettlementDlg::~SettlementDlg() {}

void SettlementDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_SETTLEMENT, m_listSettlement);
}

BOOL SettlementDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // 컬럼 설정
    m_listSettlement.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listSettlement.InsertColumn(0, _T("날짜"),      LVCFMT_LEFT,  100);
    m_listSettlement.InsertColumn(1, _T("배달건수"),  LVCFMT_CENTER, 80);
    m_listSettlement.InsertColumn(2, _T("배달료 합계"), LVCFMT_RIGHT, 100);
    m_listSettlement.InsertColumn(3, _T("상태"),       LVCFMT_CENTER, 80);

    RequestSettlement();
    return TRUE;
}

void SettlementDlg::RequestSettlement()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST);
    if (!bSent) {
        // 서버 미연결 시 더미 데이터
        struct { LPCTSTR date; int cnt; int fee; LPCTSTR status; } dummy[] = {
            { _T("2026-03-20"), 3, 10500, _T("정산예정") },
            { _T("2026-03-19"), 4, 14000, _T("정산예정") },
            { _T("2026-03-18"), 2,  7000, _T("정산완료") },
        };
        for (int i = 0; i < 3; i++) {
            CString cntStr, feeStr;
            cntStr.Format(_T("%d건"),   dummy[i].cnt);
            feeStr.Format(_T("%d원"),   dummy[i].fee);
            int nRow = m_listSettlement.InsertItem(i, dummy[i].date);
            m_listSettlement.SetItemText(nRow, 1, cntStr);
            m_listSettlement.SetItemText(nRow, 2, feeStr);
            m_listSettlement.SetItemText(nRow, 3, dummy[i].status);
        }
    }
}

LRESULT SettlementDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    int cmd = _ttoi(msg.Left(p));
    if (cmd == CMD_RIDER_MY_LIST)
        ParseAndFillList(msg.Mid(p + 1));
    return 0;
}

// payload: "date,cnt,fee,status;date,cnt,fee,status;..."
void SettlementDlg::ParseAndFillList(const CString& payload)
{
    m_listSettlement.DeleteAllItems();
    CString data = payload;
    int row = 0;

    while (!data.IsEmpty()) {
        int semi = data.Find(_T(';'));
        CString item = (semi >= 0) ? data.Left(semi) : data;
        data = (semi >= 0) ? data.Mid(semi + 1) : _T("");
        if (item.IsEmpty()) continue;

        CString date, cnt, fee, status;
        auto next = [&](CString& out) {
            int c = item.Find(_T(','));
            out = (c >= 0) ? item.Left(c) : item;
            item = (c >= 0) ? item.Mid(c + 1) : _T("");
        };
        next(date); next(cnt); next(fee); next(status);

        CString cntStr, feeStr;
        cntStr.Format(_T("%s건"), static_cast<LPCTSTR>(cnt));
        feeStr.Format(_T("%s원"), static_cast<LPCTSTR>(fee));

        int nRow = m_listSettlement.InsertItem(row++, date);
        m_listSettlement.SetItemText(nRow, 1, cntStr);
        m_listSettlement.SetItemText(nRow, 2, feeStr);
        m_listSettlement.SetItemText(nRow, 3, status);
    }
}


// ══════════════════════════════════════════════════════════════
//  DriveTimeDlg
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(DriveTimeDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DriveTimeDlg, CDialogEx)
    ON_WM_TIMER()
END_MESSAGE_MAP()

DriveTimeDlg::DriveTimeDlg(CWnd* pParent) : CDialogEx(IDD_DRIVETIME_DLG, pParent) {}
DriveTimeDlg::~DriveTimeDlg() {}

BOOL DriveTimeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 오늘 날짜 표시
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    CString dateStr;
    dateStr.Format(_T("%d월 %d일"), st.wMonth, st.wDay);
    SetDlgItemText(IDC_STATIC_TODAY_DATE, dateStr);

    // 주간 범위 표시
    CString weekRange;
    CalcWeekRange(weekRange);
    SetDlgItemText(IDC_STATIC_WEEK_RANGE, weekRange);

    // 세션의 운행 시작 시각 기준으로 현재 오늘 운행시간 계산
    if (AppContext::Get().session.isOnline) {
        // 대략적 추정: 이미 운행 중이면 다이얼로그 열린 시간 기준 누적
        m_nTodayBaseSec = 0;
    } else {
        m_nTodayBaseSec = 0;
    }
    m_dwOpenTime = GetTickCount();

    UpdateDriveTimeUI();
    SetTimer(1, 1000, nullptr);
    return TRUE;
}

void DriveTimeDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) UpdateDriveTimeUI();
    CDialogEx::OnTimer(nIDEvent);
}

void DriveTimeDlg::UpdateDriveTimeUI()
{
    // 다이얼로그 열린 이후 경과 + 기본 누적
    int elapsed = m_nTodayBaseSec;
    if (AppContext::Get().session.isOnline)
        elapsed += (int)((GetTickCount() - m_dwOpenTime) / 1000);

    SetDlgItemText(IDC_STATIC_TODAY_TIME, FormatSeconds(elapsed));
    // 주간은 오늘 * 5일 가정 (실제는 서버 데이터로 대체)
    SetDlgItemText(IDC_STATIC_WEEK_TIME,  FormatSeconds(elapsed * 5));
}

CString DriveTimeDlg::FormatSeconds(int totalSec)
{
    int h = totalSec / 3600;
    int m = (totalSec % 3600) / 60;
    CString result;
    if (h > 0)
        result.Format(_T("%d시간 %d분"), h, m);
    else
        result.Format(_T("%d분"), m);
    return result;
}

void DriveTimeDlg::CalcWeekRange(CString& outRange)
{
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    // 이번 주 월요일 ~ 일요일
    int wday = st.wDayOfWeek;  // 0=일, 1=월 ...
    int daysToMon = (wday == 0) ? -6 : 1 - wday;

    SYSTEMTIME mon = st;
    mon.wDay += daysToMon;
    SYSTEMTIME sun = mon;
    sun.wDay += 6;

    outRange.Format(_T("%d월 %d일 - %d월 %d일"),
                    mon.wMonth, mon.wDay,
                    sun.wMonth, sun.wDay);
}


// ══════════════════════════════════════════════════════════════
//  TodayHistoryDlg
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(TodayHistoryDlg, CDialogEx)

BEGIN_MESSAGE_MAP(TodayHistoryDlg, CDialogEx)
    ON_MESSAGE(WM_SOCKET_RECV, &TodayHistoryDlg::OnSocketRecv)
END_MESSAGE_MAP()

TodayHistoryDlg::TodayHistoryDlg(CWnd* pParent) : CDialogEx(IDD_TODAY_HISTORY_DLG, pParent) {}
TodayHistoryDlg::~TodayHistoryDlg() {}

void TodayHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_HISTORY, m_listHistory);
}

BOOL TodayHistoryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // 오늘 날짜 표시
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    CString dateStr;
    dateStr.Format(_T("%d월 %d일"), st.wMonth, st.wDay);
    SetDlgItemText(IDC_STATIC_TODAY_DATE, dateStr);

    // 컬럼 설정
    m_listHistory.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listHistory.InsertColumn(0, _T("주문코드"),  LVCFMT_LEFT,  90);
    m_listHistory.InsertColumn(1, _T("가게명"),    LVCFMT_LEFT,  130);
    m_listHistory.InsertColumn(2, _T("시간"),      LVCFMT_LEFT,  70);
    m_listHistory.InsertColumn(3, _T("배달료"),    LVCFMT_RIGHT, 70);

    RequestTodayHistory();
    return TRUE;
}

void TodayHistoryDlg::RequestTodayHistory()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST, _T("TODAY"));
    if (!bSent) {
        // 서버 미연결 시 더미 데이터
        struct { LPCTSTR code; LPCTSTR store; LPCTSTR time; int fee; } dummy[] = {
            { _T("2783JBCD"), _T("맛있는치킨 상무점"), _T("19:35"), 3500 },
            { _T("7824SJFE"), _T("피자헛 상무역점"),   _T("17:55"), 4000 },
            { _T("0128VPLW"), _T("버거킹 광주상무점"), _T("15:10"), 3000 },
        };
        int total = 0;
        for (int i = 0; i < 3; i++) {
            CString feeStr;
            feeStr.Format(_T("%d원"), dummy[i].fee);
            int nRow = m_listHistory.InsertItem(i, dummy[i].code);
            m_listHistory.SetItemText(nRow, 1, dummy[i].store);
            m_listHistory.SetItemText(nRow, 2, dummy[i].time);
            m_listHistory.SetItemText(nRow, 3, feeStr);
            total += dummy[i].fee;
        }
        // 합계 표시
        CString cntStr, totalStr;
        cntStr.Format(_T("3건"));
        totalStr.Format(_T("%d원"), total);
        SetDlgItemText(IDC_STATIC_TOTAL_CNT,  cntStr);
        SetDlgItemText(IDC_STATIC_TOTAL_FEE,  totalStr);
    }
}

LRESULT TodayHistoryDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    int cmd = _ttoi(msg.Left(p));
    if (cmd == CMD_RIDER_MY_LIST)
        ParseAndFill(msg.Mid(p + 1));
    return 0;
}

// payload: "code,store,time,fee;..."
void TodayHistoryDlg::ParseAndFill(const CString& payload)
{
    m_listHistory.DeleteAllItems();
    CString data = payload;
    int row = 0, total = 0, cnt = 0;

    while (!data.IsEmpty()) {
        int semi = data.Find(_T(';'));
        CString item = (semi >= 0) ? data.Left(semi) : data;
        data = (semi >= 0) ? data.Mid(semi + 1) : _T("");
        if (item.IsEmpty()) continue;

        CString code, store, timeStr, feeStr;
        auto next = [&](CString& out) {
            int c = item.Find(_T(','));
            out = (c >= 0) ? item.Left(c) : item;
            item = (c >= 0) ? item.Mid(c + 1) : _T("");
        };
        next(code); next(store); next(timeStr); next(feeStr);

        int fee = _ttoi(feeStr);
        total += fee;
        cnt++;

        CString feeDisp;
        feeDisp.Format(_T("%d원"), fee);
        int nRow = m_listHistory.InsertItem(row++, code);
        m_listHistory.SetItemText(nRow, 1, store);
        m_listHistory.SetItemText(nRow, 2, timeStr);
        m_listHistory.SetItemText(nRow, 3, feeDisp);
    }

    CString cntStr, totalStr;
    cntStr.Format(_T("%d건"), cnt);
    totalStr.Format(_T("%d원"), total);
    SetDlgItemText(IDC_STATIC_TOTAL_CNT, cntStr);
    SetDlgItemText(IDC_STATIC_TOTAL_FEE, totalStr);
}

void SettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

void DriveTimeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}
