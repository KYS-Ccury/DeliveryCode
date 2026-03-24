// ================================================================
//  MainHomeDlg.cpp  ─  메인 홈 화면  (서버 연동 완성본)
//
//  [연동 프로토콜]
//    REQ : CmdCustomer::REQ_STORE_LIST (200)
//          { "category":"치킨" }   ← 빈 문자열이면 전체 조회
//    RES : { "status":2000,
//            "stores":[
//              { "store_id":101, "name":"황금치킨",
//                "category":"치킨", "delivery_time":"25~35분",
//                "min_order":18000, "distance":1.2,
//                "delivery_fee":"2000~3000" },
//              ...
//            ] }
//
//  [흐름]
//    OnInitDialog → SendStoreListRequest(빈 문자열=전체)
//    → OnScrollMenuClicked → SendStoreListRequest(카테고리)
//    → OnStoreListResponse(WM_USER+110) → UpdateStoreListUI()
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MainHomeDlg.h"
#include "OrderManager.h"
#include "CartDlg.h"
#include "StoreListDlg.h"
#include "OrderHistoryDlg.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

#define WM_STORE_LIST_RESPONSE (WM_USER + 110)

// ── 간이 JSON 파싱 헬퍼 ───────────────────────────────────────
static std::string ExtractJsonStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static int ExtractJsonInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return 0;
    pos += token.size();
    try { return std::stoi(json.substr(pos)); } catch (...) { return 0; }
}
static double ExtractJsonDouble(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return 0.0;
    pos += token.size();
    try { return std::stod(json.substr(pos)); } catch (...) { return 0.0; }
}

// ── JSON 배열에서 StoreInfo 목록 파싱 ────────────────────────
static std::vector<StoreInfo> ParseStoreArray(const std::string& json)
{
    std::vector<StoreInfo> stores;
    // "stores":[{...},{...}] 영역 추출
    auto arrStart = json.find("\"stores\":[");
    if (arrStart == std::string::npos) return stores;
    arrStart += 10; // "stores":[ 길이

    size_t i = arrStart;
    while (i < json.size()) {
        auto objStart = json.find('{', i);
        if (objStart == std::string::npos) break;
        // 중첩 없는 단순 객체 기준으로 닫는 } 찾기
        int depth = 0;
        size_t objEnd = objStart;
        for (; objEnd < json.size(); ++objEnd) {
            if (json[objEnd] == '{') ++depth;
            else if (json[objEnd] == '}') { if (--depth == 0) break; }
        }
        std::string obj = json.substr(objStart, objEnd - objStart + 1);

        StoreInfo s;
        s.storeID           = ExtractJsonInt(obj, "store_id");
        s.storeName         = ExtractJsonStr(obj, "name");
        s.category          = ExtractJsonStr(obj, "category");
        s.deliveryTime      = ExtractJsonStr(obj, "delivery_time");
        s.deliveryPriceRange = ExtractJsonStr(obj, "delivery_fee");
        s.address           = ExtractJsonStr(obj, "address");
        s.openTime          = ExtractJsonStr(obj, "open_time");
        s.phoneNumber       = ExtractJsonStr(obj, "phone");
        s.holiday           = ExtractJsonStr(obj, "holiday");
        s.minOrderAmount    = ExtractJsonInt(obj, "min_order");
        s.distance          = ExtractJsonDouble(obj, "distance");
        if (s.storeID > 0) stores.push_back(s);
        i = objEnd + 1;
    }
    return stores;
}

IMPLEMENT_DYNAMIC(MainHomeDlg, CDialogEx)

MainHomeDlg::MainHomeDlg(CWnd* pParent)
    : CDialogEx(IDD_MAINHOME_DLG, pParent) {}
MainHomeDlg::~MainHomeDlg() {}

void MainHomeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_STOR, m_listStore);
}

BEGIN_MESSAGE_MAP(MainHomeDlg, CDialogEx)
    ON_WM_MOUSEWHEEL()
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED,   &MainHomeDlg::OnScrollMenuClicked)
    ON_MESSAGE(WM_STORE_LIST_RESPONSE,   &MainHomeDlg::OnStoreListResponse)
    ON_BN_CLICKED(IDC_BUTTON1,           &MainHomeDlg::OnBnClickedButton1)
    ON_NOTIFY(NM_CLICK, IDC_LIST_STOR,   &MainHomeDlg::OnNMDblclkListStor)
    ON_BN_CLICKED(IDC_BTN_MYPAGE,        &MainHomeDlg::OnBnClickedBtnOrderHistory)
END_MESSAGE_MAP()

BOOL MainHomeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN | WS_DLGFRAME);
    CenterWindow();

    // ── 리스트 컬럼 설정 ──────────────────────────────────────
    m_listStore.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listStore.InsertColumn(0, _T("매장명"),       LVCFMT_LEFT,   200);
    m_listStore.InsertColumn(1, _T("배달시간"),     LVCFMT_CENTER, 100);
    m_listStore.InsertColumn(2, _T("최소주문금액"), LVCFMT_RIGHT,  120);
    m_listStore.InsertColumn(3, _T("거리"),         LVCFMT_CENTER,  60);

    // ── CScrollMenu 수동 Create ───────────────────────────────
    CWnd* pPlaceholder = GetDlgItem(IDC_STATIC_MENU_BAR);
    if (pPlaceholder) {
        CRect rect;
        pPlaceholder->GetWindowRect(&rect);
        ScreenToClient(&rect);
        pPlaceholder->ShowWindow(SW_HIDE);
        m_wndScrollMenu.Create(
            _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_NOTIFY,
            rect, this, IDC_STATIC_MENU_BAR);
    }

    // ── 카테고리 목록 설정 ────────────────────────────────────
    m_vecCategories = {
        _T("전체"), _T("족발/보쌈"), _T("찜/탕"), _T("일식"),
        _T("치킨"), _T("피자"),     _T("중식"),   _T("양식")
    };
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecCategories);

    m_brushBack.CreateSolidBrush(RGB(230, 245, 245));
    m_brushWhite.CreateSolidBrush(RGB(255, 255, 255));

    // ── 서버 연동: 전체 가게 목록 요청 ───────────────────────
    RegisterNetworkCallback();
    SendStoreListRequest(_T("전체"));

    return TRUE;
}

// ── 네트워크 콜백 등록 ────────────────────────────────────────
void MainHomeDlg::RegisterNetworkCallback()
{
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdCustomer::REQ_STORE_LIST,
        [this](uint16_t, const std::string& body) {
            // ReceiveLoop 스레드에서 호출됨 → PostMessage로 UI 전달
            // body를 힙에 복사해서 lParam으로 전달
            std::string* pBody = new std::string(body);
            PostMessage(WM_STORE_LIST_RESPONSE, 0, (LPARAM)pBody);
        });
}

// ── 서버에 가게 목록 요청 ─────────────────────────────────────
void MainHomeDlg::SendStoreListRequest(const CString& category)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 서버 연결 없으면 로컬 더미 데이터 사용
        OrderManager::GetInstance().LoadStoreData();
        UpdateStoreListUI(category);
        return;
    }

    std::string cat = (category == _T("전체")) ? "" : std::string(CT2A(category, CP_UTF8));
    std::string json = "{\"category\":\"" + cat + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_STORE_LIST, json);

    // 로딩 표시
    m_listStore.DeleteAllItems();
    m_listStore.InsertItem(0, _T("가게 목록을 불러오는 중..."));
}

// ── 서버 응답 처리 (UI 스레드) ────────────────────────────────
LRESULT MainHomeDlg::OnStoreListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = 0;
    {
        std::string token = "\"status\":";
        auto pos = pBody->find(token);
        if (pos != std::string::npos)
            try { status = std::stoi(pBody->substr(pos + token.size())); } catch (...) {}
    }

    if (status == (int)Status::SUCCESS) {
        m_vecStoreCache = ParseStoreArray(*pBody);
        // OrderManager 캐시 갱신
        OrderManager::GetInstance().UpdateStoreCache(m_vecStoreCache);
        RebuildStoreListUI(m_vecStoreCache);
    } else {
        m_listStore.DeleteAllItems();
        m_listStore.InsertItem(0, _T("가게 정보를 가져오지 못했습니다."));
    }

    delete pBody;
    return 0;
}

// ── UI 갱신 (StoreInfo 벡터 직접 사용) ───────────────────────
void MainHomeDlg::RebuildStoreListUI(const std::vector<StoreInfo>& stores)
{
    m_listStore.DeleteAllItems();
    for (int i = 0; i < (int)stores.size(); ++i) {
        CString strName = CA2T(stores[i].storeName.c_str(), CP_UTF8);
        CString strTime = CA2T(stores[i].deliveryTime.c_str(), CP_UTF8);
        int nRow = m_listStore.InsertItem(i, strName);
        m_listStore.SetItemText(nRow, 1, strTime);
        CString strAmt; strAmt.Format(_T("%d원"), stores[i].minOrderAmount);
        m_listStore.SetItemText(nRow, 2, strAmt);
        CString strDist; strDist.Format(_T("%.1fkm"), stores[i].distance);
        m_listStore.SetItemText(nRow, 3, strDist);
    }
    if (stores.empty())
        m_listStore.InsertItem(0, _T("해당 카테고리의 가게가 없습니다."));
}

// 기존 OrderManager 기반 로컬 갱신 (폴백용)
void MainHomeDlg::UpdateStoreListUI(CString categoryName)
{
    m_listStore.DeleteAllItems();
    m_vecStoreCache.clear();
    std::string targetCat = std::string(CT2A(categoryName, CP_UTF8));
    auto stores = OrderManager::GetInstance().GetStoresByCategory(targetCat);
    RebuildStoreListUI(stores);
    m_vecStoreCache = stores;
}

// ── 카테고리 탭 클릭 ─────────────────────────────────────────
LRESULT MainHomeDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM lParam)
{
    int nIndex = (UINT)wParam - 2000;
    if (nIndex >= 0 && nIndex < (int)m_vecCategories.size())
        SendStoreListRequest(m_vecCategories[nIndex]);
    return 0;
}

// ── 가게 클릭 → StoreListDlg ─────────────────────────────────
void MainHomeDlg::OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int nIndex = pNMIA->iItem;
    if (nIndex >= 0 && nIndex < (int)m_vecStoreCache.size()) {
        const StoreInfo& si = m_vecStoreCache[nIndex];
        OrderManager::GetInstance().SelectStore(si.storeID);

        StoreListDlg dlg(this);
        dlg.m_strStoreName = CA2T(si.storeName.c_str(), CP_UTF8);
        dlg.m_storeInfo    = si;
        dlg.DoModal();
    }
    *pResult = 0;
}

// ── 장바구니 버튼 ─────────────────────────────────────────────
void MainHomeDlg::OnBnClickedButton1()
{
    CartDlg dlg(this);
    dlg.DoModal();
}

// ── 마이페이지 / 주문내역 버튼 ───────────────────────────────
void MainHomeDlg::OnBnClickedBtnOrderHistory()
{
    OrderHistoryDlg dlg(this);
    dlg.DoModal();
}

BOOL MainHomeDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (m_wndScrollMenu.GetSafeHwnd()) {
        CRect rect;
        m_wndScrollMenu.GetWindowRect(&rect);
        if (rect.PtInRect(pt))
            return m_wndScrollMenu.OnMouseWheel(nFlags, zDelta, pt);
    }
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

HBRUSH MainHomeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_DLG) return m_brushBack;
    if (nCtlColor == CTLCOLOR_STATIC) {
        pDC->SetBkMode(TRANSPARENT);
        return m_brushWhite;
    }
    return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}
