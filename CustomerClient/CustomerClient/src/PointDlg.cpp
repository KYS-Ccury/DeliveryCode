// ================================================================
//  PointDlg.cpp  ─  포인트 확인
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "PointDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "OrderManager.h"
#include "common/header/Types.h"

// ── 간이 JSON 파싱 ────────────────────────────────────────────
static int PTJInt(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return -1;
    try { return std::stoi(j.substr(p + t.size())); } catch (...) { return -1; }
}
static std::string PTJStr(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":\"";
    auto p = j.find(t); if (p == std::string::npos) return "";
    p += t.size(); auto e = j.find('"', p);
    return (e == std::string::npos) ? "" : j.substr(p, e - p);
}
static std::vector<std::string> PTExtractObjects(const std::string& json,
                                                  const std::string& key)
{
    std::vector<std::string> result;
    std::string token = "\"" + key + "\":[";
    auto arrPos = json.find(token);
    if (arrPos == std::string::npos) return result;
    size_t i = arrPos + token.size();
    while (i < json.size()) {
        auto s = json.find('{', i); if (s == std::string::npos) break;
        int d = 0; size_t e = s;
        for (; e < json.size(); ++e) {
            if (json[e] == '{') ++d;
            else if (json[e] == '}') { if (--d == 0) break; }
        }
        result.push_back(json.substr(s, e - s + 1));
        i = e + 1;
    }
    return result;
}

// =================================================================

IMPLEMENT_DYNAMIC(PointDlg, CDialogEx)

PointDlg::PointDlg(CWnd* pParent)
    : CDialogEx(IDD_POINT_DLG, pParent)
{}

PointDlg::~PointDlg() {}

void PointDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_POINT_HISTORY, m_listHistory);
}

BEGIN_MESSAGE_MAP(PointDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,  &PointDlg::OnBnClickedBtnBack)
    ON_MESSAGE(WM_POINT_RESPONSE, &PointDlg::OnPointResponse)
END_MESSAGE_MAP()

BOOL PointDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // ── 리스트뷰 컬럼 ─────────────────────────────────────
    m_listHistory.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listHistory.InsertColumn(0, _T("날짜"),   LVCFMT_CENTER,  90);
    m_listHistory.InsertColumn(1, _T("내용"),   LVCFMT_LEFT,   160);
    m_listHistory.InsertColumn(2, _T("포인트"), LVCFMT_RIGHT,   70);

    // ── 보유 포인트 초기 표시 (OrderManager 캐시) ─────────
    int cached = OrderManager::GetInstance().GetMyPoints();
    CString strPt;
    strPt.Format(_T("보유 포인트 : %d P"), cached);
    SetDlgItemText(IDC_STATIC_MY_POINT_TOTAL, strPt);

    // ── 서버 콜백 등록 ────────────────────────────────────
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(
        CmdCustomer::REQ_MY_POINT,
        [hThis](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            ::PostMessage(hThis, WM_POINT_RESPONSE, 0, (LPARAM)p);
        });

    RequestPoint();
    return TRUE;
}

void PointDlg::RequestPoint()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        m_listHistory.InsertItem(0, _T("서버에 연결되어 있지 않습니다."));
        return;
    }
    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json  = "{\"token\":\"" + token + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_MY_POINT, json);

    SetDlgItemText(IDC_STATIC_MY_POINT_TOTAL, _T("포인트 조회 중..."));
}

LRESULT PointDlg::OnPointResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = PTJInt(*pBody, "status");
    if (status == (int)Status::SUCCESS) {
        int points = PTJInt(*pBody, "points");

        // OrderManager에 최신 포인트 저장
        OrderManager::GetInstance().SetMyPoints(points);

        CString strPt;
        strPt.Format(_T("보유 포인트 : %d P"), points);
        SetDlgItemText(IDC_STATIC_MY_POINT_TOTAL, strPt);

        // 포인트 내역 파싱
        m_listHistory.DeleteAllItems();
        auto histObjs = PTExtractObjects(*pBody, "history");
        for (int i = 0; i < (int)histObjs.size(); ++i) {
            const auto& h = histObjs[i];
            CString strDate = CA2T(PTJStr(h, "date").c_str(), CP_UTF8);
            CString strDesc = CA2T(PTJStr(h, "desc").c_str(), CP_UTF8);
            int     amount  = PTJInt(h, "amount");

            int r = m_listHistory.InsertItem(i, strDate);
            m_listHistory.SetItemText(r, 1, strDesc);

            CString strAmt;
            if (amount >= 0)
                strAmt.Format(_T("+%d P"), amount);
            else
                strAmt.Format(_T("%d P"), amount);
            m_listHistory.SetItemText(r, 2, strAmt);
        }

        if (histObjs.empty())
            m_listHistory.InsertItem(0, _T("포인트 내역이 없습니다."));

    } else {
        SetDlgItemText(IDC_STATIC_MY_POINT_TOTAL, _T("포인트 조회 실패"));
    }

    delete pBody;
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MY_POINT);
    return 0;
}

void PointDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MY_POINT);
    EndDialog(IDCANCEL);
}
