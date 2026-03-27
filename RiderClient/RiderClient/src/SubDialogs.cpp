// SubDialogs.cpp - SettingsDlg, SettlementDlg, DriveTimeDlg, TodayHistoryDlg
// Fixed: OnCtlColor text visibility, JSON protocol for SendPacket calls
#include "pch.h"
#include "SubDialogs.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
#include <map>
#include <vector>
using json = nlohmann::json;

// Shared OnCtlColor helper macro - avoids code duplication
#define IMPL_CTLCOLOR(ClassName) \
HBRUSH ClassName::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) \
{ \
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor); \
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) { \
        if (!m_hBrushBg) m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242)); \
        pDC->SetBkColor(RGB(225, 248, 242)); \
        pDC->SetTextColor(RGB(10, 10, 10)); \
        return m_hBrushBg; \
    } \
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) { \
        pDC->SetBkColor(RGB(255, 255, 255)); \
        pDC->SetTextColor(RGB(10, 10, 10)); \
        return (HBRUSH)GetStockObject(WHITE_BRUSH); \
    } \
    return hbr; \
}

// ================================================================
//  SettingsDlg
// ================================================================
IMPLEMENT_DYNAMIC(SettingsDlg, CDialogEx)

BEGIN_MESSAGE_MAP(SettingsDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_REGION_NEWS,   &SettingsDlg::OnBtnRegionNews)
    ON_BN_CLICKED(IDC_BTN_DISPATCH_TYPE, &SettingsDlg::OnBtnDispatchType)
    ON_BN_CLICKED(IDOK,                  &SettingsDlg::OnBtnSave)
END_MESSAGE_MAP()

SettingsDlg::SettingsDlg(CWnd* pParent) : CDialogEx(IDD_SETTINGS_DLG, pParent) {}
SettingsDlg::~SettingsDlg() {}

void SettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL SettingsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    LoadSettings();
    SetDlgItemText(IDC_EDIT_REGION_NEWS,     m_strRegionNews);
    SetDlgItemText(IDC_STATIC_DISPATCH_TYPE, m_strDispatchType);
    return TRUE;
}

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

void SettingsDlg::OnBtnDispatchType()
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1001, _T("자동배차 (가까운 주문 자동 수락)"));
    menu.AppendMenu(MF_STRING, 1002, _T("수동배차 (요청을 직접 수락/거절)"));

    CWnd* pBtn = GetDlgItem(IDC_BTN_DISPATCH_TYPE);
    CRect rc; pBtn->GetWindowRect(&rc);

    int sel = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
                                  rc.left, rc.bottom, this);
    if (sel == 1001)      m_strDispatchType = _T("자동배차 (가까운 주문 자동 수락)");
    else if (sel == 1002) m_strDispatchType = _T("수동배차 (요청을 직접 수락/거절)");
    if (sel != 0) SetDlgItemText(IDC_STATIC_DISPATCH_TYPE, m_strDispatchType);
}

void SettingsDlg::OnBtnSave() { SaveSettings(); EndDialog(IDOK); }

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
        m_strRegionNews   = AppContext::Get().session.deliveryRegion;
        m_strDispatchType = _T("수동배차 (요청을 직접 수락/거절)");
        return;
    }
    auto readStr = [&](LPCTSTR name, CString& out, LPCTSTR def) {
        TCHAR buf[256] = {};
        DWORD size = sizeof(buf), type = REG_SZ;
        if (RegQueryValueEx(hKey, name, nullptr, &type,
                            reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS)
            out = buf;
        else out = def;
    };
    readStr(_T("RegionNews"),   m_strRegionNews,
            static_cast<LPCTSTR>(AppContext::Get().session.deliveryRegion));
    readStr(_T("DispatchType"), m_strDispatchType, _T("수동배차 (요청을 직접 수락/거절)"));
    RegCloseKey(hKey);
}

IMPL_CTLCOLOR(SettingsDlg)


// ================================================================
//  SettlementDlg
// ================================================================
IMPLEMENT_DYNAMIC(SettlementDlg, CDialogEx)

BEGIN_MESSAGE_MAP(SettlementDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_SOCKET_RECV, &SettlementDlg::OnSocketRecv)
END_MESSAGE_MAP()

SettlementDlg::SettlementDlg(CWnd* pParent) : CDialogEx(IDD_SETTLEMENT_DLG, pParent) {}
SettlementDlg::~SettlementDlg() {
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_MY_LIST);
}

void SettlementDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_SETTLEMENT, m_listSettlement);
}

BOOL SettlementDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_MY_LIST, GetSafeHwnd());

    m_listSettlement.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listSettlement.InsertColumn(0, _T("날짜"),        LVCFMT_LEFT,   100);
    m_listSettlement.InsertColumn(1, _T("배달건수"),       LVCFMT_CENTER,  80);
    m_listSettlement.InsertColumn(2, _T("배달료 합계"),   LVCFMT_RIGHT,  100);
    m_listSettlement.InsertColumn(3, _T("상태"),      LVCFMT_CENTER,  80);

    RequestSettlement();
    return TRUE;
}

// CMD_RIDER_MY_LIST (405), {"summary_only":false}
void SettlementDlg::RequestSettlement()
{
    json req; req["summary_only"] = false;
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST, req.dump());
    if (!bSent) {
        struct { LPCTSTR date; int cnt; int fee; LPCTSTR status; } dummy[] = {
            { _T("2026-03-20"), 3, 10500, _T("정산예정") },
            { _T("2026-03-19"), 4, 14000, _T("정산예정") },
            { _T("2026-03-18"), 2,  7000, _T("완료") },
        };
        for (int i = 0; i < 3; i++) {
            CString cntStr, feeStr;
            cntStr.Format(_T("%d"), dummy[i].cnt);
            feeStr.Format(_T("%d원"), dummy[i].fee);
            int nRow = m_listSettlement.InsertItem(i, dummy[i].date);
            m_listSettlement.SetItemText(nRow, 1, cntStr);
            m_listSettlement.SetItemText(nRow, 2, feeStr);
            m_listSettlement.SetItemText(nRow, 3, dummy[i].status);
        }
    }
}

// Recv 405: {"status":2000,"records":[{order_id,store_name,delivery_fee,created_at,...},...]}
LRESULT SettlementDlg::OnSocketRecv(WPARAM, LPARAM lParam)
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

        // 날짜별로 집계 (key: "MM/DD" 앞 5자)
        // records: [{order_id, store_name, delivery_fee, status, created_at}, ...]
        std::map<std::string, std::pair<int,int>> dateMap; // date -> (건수, 합계)
        std::vector<std::string> dateOrder; // 날짜 순서 유지

        for (const auto& r : res["records"]) {
            std::string createdAt = r.value("created_at", "");
            // created_at 형식: "MM/DD HH:MM" — 날짜 부분(앞 5자) 추출
            std::string date = createdAt.size() >= 5 ? createdAt.substr(0, 5) : createdAt;
            int fee = r.value("delivery_fee", 0);

            if (dateMap.find(date) == dateMap.end()) {
                dateOrder.push_back(date);
                dateMap[date] = {0, 0};
            }
            dateMap[date].first++;       // 건수 +1
            dateMap[date].second += fee; // 합계 누적
        }

        m_listSettlement.DeleteAllItems();
        int row = 0;
        for (const auto& date : dateOrder) {
            auto& v = dateMap[date];
            CA2T dateW(date.c_str(), CP_UTF8);
            CString cntStr, feeStr;
            cntStr.Format(_T("%d건"), v.first);
            feeStr.Format(_T("%d원"), v.second);
            int nRow = m_listSettlement.InsertItem(row++, CString(dateW));
            m_listSettlement.SetItemText(nRow, 1, cntStr);
            m_listSettlement.SetItemText(nRow, 2, feeStr);
            m_listSettlement.SetItemText(nRow, 3, _T("정산예정"));
        }
    } catch (...) {}
    return 0;
}

IMPL_CTLCOLOR(SettlementDlg)


// ================================================================
//  DriveTimeDlg
// ================================================================
IMPLEMENT_DYNAMIC(DriveTimeDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DriveTimeDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_WM_TIMER()
END_MESSAGE_MAP()

DriveTimeDlg::DriveTimeDlg(CWnd* pParent) : CDialogEx(IDD_DRIVETIME_DLG, pParent) {}
DriveTimeDlg::~DriveTimeDlg() {}

void DriveTimeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL DriveTimeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    CString dateStr;
    dateStr.Format(_T("%d/%d"), st.wMonth, st.wDay);
    SetDlgItemText(IDC_STATIC_TODAY_DATE, dateStr);

    CString weekRange;
    CalcWeekRange(weekRange);
    SetDlgItemText(IDC_STATIC_WEEK_RANGE, weekRange);

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
    const RiderSession& sess = AppContext::Get().session;
    ULONGLONG totalSec = sess.totalDriveSec;
    // drivingStartTick이 있으면 현재 운행 중 - isOnline 무관하게 계산
    if (sess.drivingStartTick > 0)
        totalSec += (GetTickCount64() - sess.drivingStartTick) / 1000;
    SetDlgItemText(IDC_STATIC_TODAY_TIME, FormatSeconds((int)totalSec));
    SetDlgItemText(IDC_STATIC_WEEK_TIME,  FormatSeconds((int)totalSec));
}

CString DriveTimeDlg::FormatSeconds(int totalSec)
{
    int h = totalSec / 3600;
    int m = (totalSec % 3600) / 60;
    int s = totalSec % 60;
    CString result;
    if (h > 0)       result.Format(_T("%d시간 %d분 %02d초"), h, m, s);
    else if (m > 0)  result.Format(_T("%d분 %02d초"), m, s);
    else             result.Format(_T("%d초"), s);
    return result;
}

void DriveTimeDlg::CalcWeekRange(CString& outRange)
{
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    int wday = st.wDayOfWeek;
    int daysToMon = (wday == 0) ? -6 : 1 - wday;
    SYSTEMTIME mon = st; mon.wDay += daysToMon;
    SYSTEMTIME sun = mon; sun.wDay += 6;
    outRange.Format(_T("%d/%d - %d/%d"), mon.wMonth, mon.wDay, sun.wMonth, sun.wDay);
}

IMPL_CTLCOLOR(DriveTimeDlg)


// ================================================================
//  TodayHistoryDlg
// ================================================================
IMPLEMENT_DYNAMIC(TodayHistoryDlg, CDialogEx)

BEGIN_MESSAGE_MAP(TodayHistoryDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_SOCKET_RECV, &TodayHistoryDlg::OnSocketRecv)
END_MESSAGE_MAP()

TodayHistoryDlg::TodayHistoryDlg(CWnd* pParent) : CDialogEx(IDD_TODAY_HISTORY_DLG, pParent) {}
TodayHistoryDlg::~TodayHistoryDlg() {
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_MY_LIST);
}

void TodayHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_HISTORY, m_listHistory);
}

BOOL TodayHistoryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_MY_LIST, GetSafeHwnd());

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    CString dateStr;
    dateStr.Format(_T("%d/%d"), st.wMonth, st.wDay);
    SetDlgItemText(IDC_STATIC_TODAY_DATE, dateStr);

    m_listHistory.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listHistory.InsertColumn(0, _T("주문코드"), LVCFMT_LEFT,  90);
    m_listHistory.InsertColumn(1, _T("가게명"),      LVCFMT_LEFT, 130);
    m_listHistory.InsertColumn(2, _T("시간"),       LVCFMT_LEFT,  70);
    m_listHistory.InsertColumn(3, _T("배달료"),        LVCFMT_RIGHT, 70);

    RequestTodayHistory();
    return TRUE;
}

// CMD_RIDER_MY_LIST (405), {"summary_only":false,"today_only":true}
void TodayHistoryDlg::RequestTodayHistory()
{
    json req; req["summary_only"] = false; req["today_only"] = true;
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST, req.dump());
    if (!bSent) {
        struct { LPCTSTR code; LPCTSTR store; LPCTSTR time; int fee; } dummy[] = {
            { _T("ORD000001"), _T("상무치킨"), _T("19:35"), 3500 },
            { _T("ORD000002"), _T("피자헛 상무점"), _T("17:55"), 4000 },
            { _T("ORD000003"), _T("버거킹 상무점"), _T("15:10"), 3000 },
        };
        int total = 0;
        for (int i = 0; i < 3; i++) {
            CString feeStr; feeStr.Format(_T("%d원"), dummy[i].fee);
            int nRow = m_listHistory.InsertItem(i, dummy[i].code);
            m_listHistory.SetItemText(nRow, 1, dummy[i].store);
            m_listHistory.SetItemText(nRow, 2, dummy[i].time);
            m_listHistory.SetItemText(nRow, 3, feeStr);
            total += dummy[i].fee;
        }
        CString cntStr, totalStr;
        cntStr.Format(_T("3")); totalStr.Format(_T("%d원"), total);
        SetDlgItemText(IDC_STATIC_TOTAL_CNT, cntStr);
        SetDlgItemText(IDC_STATIC_TOTAL_FEE, totalStr);
    }
}

// Recv 405: same as SettlementDlg but filtered to today
LRESULT TodayHistoryDlg::OnSocketRecv(WPARAM, LPARAM lParam)
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

        m_listHistory.DeleteAllItems();
        int row = 0, total = 0;
        for (const auto& r : res["records"]) {
            int fee = r.value("delivery_fee", 0);
            total += fee;
            CA2T codeW(r.value("order_code","").c_str(), CP_UTF8);
            CA2T storeW(r.value("store_name","").c_str(), CP_UTF8);
            CA2T dateW(r.value("created_at","").c_str(), CP_UTF8);
            CString feeStr; feeStr.Format(_T("%d원"), fee);
            int nRow = m_listHistory.InsertItem(row++, CString(codeW));
            m_listHistory.SetItemText(nRow, 1, CString(storeW));
            m_listHistory.SetItemText(nRow, 2, CString(dateW));
            m_listHistory.SetItemText(nRow, 3, feeStr);
        }
        CString cntStr, totalStr;
        cntStr.Format(_T("%d"), row);
        totalStr.Format(_T("%d원"), total);
        SetDlgItemText(IDC_STATIC_TOTAL_CNT, cntStr);
        SetDlgItemText(IDC_STATIC_TOTAL_FEE, totalStr);
    } catch (...) {}
    return 0;
}

IMPL_CTLCOLOR(TodayHistoryDlg)
