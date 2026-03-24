// ================================================================
//  StoreListDlg.cpp  ─  가게 상세 / 메뉴 목록 화면  (서버 연동 완성본)
//
//  [연동 프로토콜]
//    REQ : CmdCustomer::REQ_MENU_LIST (201)
//          { "store_id":101, "category":"인기메뉴" }
//          category 가 "전체" 이면 빈 문자열로 전송
//
//    RES : { "status":2000,
//            "menus":[
//              { "menu_id":1, "name":"황금치킨", "price":18000,
//                "description":"바삭하고 촉촉한...",
//                "sub_category":"인기메뉴",
//                "options":[
//                  { "group_name":"맵기 선택", "required":true,
//                    "items":[
//                      {"option_id":1,"name":"보통맛","price":0},
//                      {"option_id":2,"name":"매운맛","price":0}
//                    ]
//                  }
//                ]
//              }, ...
//            ]
//          }
//
//  [흐름]
//    OnInitDialog → SendMenuListRequest("전체")
//    → OnScrollMenuClicked → SendMenuListRequest(subCategory)
//    → WM_MENU_LIST_RESPONSE → OnMenuListResponse() → UpdateMenuListUI()
//    → NM_CLICK → MenuDetailDlg (메뉴 객체 전달)
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "StoreListDlg.h"
#include "OrderManager.h"
#include "CartDlg.h"
#include "StoreDetailDlg.h"
#include "MenuDetailDlg.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

#define WM_MENU_LIST_RESPONSE (WM_USER + 120)

// ── 간이 JSON 파싱 헬퍼 ───────────────────────────────────────
static std::string JStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static int JInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return 0;
    pos += token.size();
    try { return std::stoi(json.substr(pos)); } catch (...) { return 0; }
}
static bool JBool(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos += token.size();
    return json.substr(pos, 4) == "true";
}

// ── JSON에서 객체 1개 추출 (depth 추적) ──────────────────────
static std::string ExtractObject(const std::string& json, size_t start)
{
    int depth = 0;
    for (size_t i = start; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}') { if (--depth == 0) return json.substr(start, i - start + 1); }
    }
    return "";
}

// ── JSON에서 배열 원소(객체) 목록 추출 ───────────────────────
static std::vector<std::string> ExtractArray(const std::string& json, const std::string& arrayKey)
{
    std::vector<std::string> elems;
    std::string token = "\"" + arrayKey + "\":[";
    auto arrPos = json.find(token);
    if (arrPos == std::string::npos) return elems;
    size_t i = arrPos + token.size();
    while (i < json.size()) {
        auto objStart = json.find('{', i);
        if (objStart == std::string::npos) break;
        std::string obj = ExtractObject(json, objStart);
        if (!obj.empty()) elems.push_back(obj);
        i = objStart + obj.size();
    }
    return elems;
}

// ── JSON → MenuInfo 파싱 ─────────────────────────────────────
static MenuInfo ParseMenuObject(const std::string& obj)
{
    MenuInfo m;
    m.menuID      = JInt(obj, "menu_id");
    m.menuName    = JStr(obj, "name");
    m.price       = JInt(obj, "price");
    m.subCategory = JStr(obj, "sub_category");
    m.menuImageUrl = JStr(obj, "image_url");

    // 옵션 그룹 파싱
    auto groups = ExtractArray(obj, "options");
    for (const auto& grpJson : groups) {
        OptionGroup og;
        og.groupName  = JStr(grpJson, "group_name");
        og.isRequired = JBool(grpJson, "required");

        auto items = ExtractArray(grpJson, "items");
        for (const auto& itemJson : items) {
            OptionItem oi;
            oi.optionID    = JInt(itemJson, "option_id");
            oi.optionName  = JStr(itemJson, "name");
            oi.optionPrice = JInt(itemJson, "price");
            og.items.push_back(oi);
        }
        m.optionGroups.push_back(og);
    }
    return m;
}

// ── MenuInfo 목록 파싱 ────────────────────────────────────────
static std::vector<MenuInfo> ParseMenuArray(const std::string& body)
{
    std::vector<MenuInfo> result;
    auto menuObjs = ExtractArray(body, "menus");
    for (const auto& obj : menuObjs) {
        MenuInfo m = ParseMenuObject(obj);
        if (m.menuID > 0) result.push_back(m);
    }
    return result;
}

// =================================================================

IMPLEMENT_DYNAMIC(StoreListDlg, CDialogEx)

StoreListDlg::StoreListDlg(CWnd* pParent)
    : CDialogEx(IDD_STORELIST_DLG, pParent) {}
StoreListDlg::~StoreListDlg() {}

void StoreListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_MENU_ITEMS,     m_listMenu);
    DDX_Control(pDX, IDC_STATIC_SUB_MENU_BAR, m_wndScrollMenu);
}

BEGIN_MESSAGE_MAP(StoreListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,              &StoreListDlg::OnBnClickedBtnBack)
    ON_WM_MOUSEWHEEL()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED,       &StoreListDlg::OnScrollMenuClicked)
    ON_MESSAGE(WM_MENU_LIST_RESPONSE,        &StoreListDlg::OnMenuListResponse)
    ON_BN_CLICKED(IDC_BTN_CART,              &StoreListDlg::OnBnClickedBtnCart)
    ON_BN_CLICKED(IDC_BTN_STORE_INFO,        &StoreListDlg::OnBnClickedBtnStoreInfo)
    ON_NOTIFY(NM_CLICK, IDC_LIST_MENU_ITEMS, &StoreListDlg::OnNMClickListMenuItems)
END_MESSAGE_MAP()

BOOL StoreListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_CAPTION, 0);
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN);
    CenterWindow();

    // ── 리스트 설정 ───────────────────────────────────────────
    m_listMenu.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMenu.InsertColumn(0, _T("메뉴명"), LVCFMT_LEFT,  180);
    m_listMenu.InsertColumn(1, _T("설명"),   LVCFMT_LEFT,  200);
    m_listMenu.InsertColumn(2, _T("가격"),   LVCFMT_RIGHT,  80);

    if (!m_strStoreName.IsEmpty()) SetWindowText(m_strStoreName);

    // ── 서브카테고리 탭 ───────────────────────────────────────
    // 일단 기본값 설정; 서버 응답에 sub_category 종류가 오면 동적으로 교체
    m_vecSubCategories = { _T("전체"), _T("인기메뉴"), _T("세트메뉴"), _T("단품"), _T("음료") };
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecSubCategories);

    // ── 네트워크 콜백 등록 후 서버 요청 ─────────────────────
    RegisterMenuCallback();
    SendMenuListRequest(_T("전체"));

    return TRUE;
}

// ── 네트워크 콜백 등록 ────────────────────────────────────────
void StoreListDlg::RegisterMenuCallback()
{
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdCustomer::REQ_MENU_LIST,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            PostMessage(WM_MENU_LIST_RESPONSE, 0, (LPARAM)pBody);
        });
}

// ── 서버에 메뉴 목록 요청 ────────────────────────────────────
void StoreListDlg::SendMenuListRequest(const CString& subCategory)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 서버 없으면 로컬 더미 데이터 사용
        UpdateMenuListUI(subCategory);
        return;
    }

    std::string cat = (subCategory == _T("전체"))
        ? "" : std::string(CT2A(subCategory, CP_UTF8));

    std::string json = "{\"store_id\":" + std::to_string(m_storeInfo.storeID)
                     + ",\"category\":\"" + cat + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_MENU_LIST, json);

    m_listMenu.DeleteAllItems();
    m_listMenu.InsertItem(0, _T("메뉴를 불러오는 중..."));
}

// ── 서버 응답 처리 (UI 스레드) ────────────────────────────────
LRESULT StoreListDlg::OnMenuListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = JInt(*pBody, "status");

    if (status == (int)Status::SUCCESS) {
        m_vecMenuCache = ParseMenuArray(*pBody);

        // 서브카테고리 목록 동적 갱신
        std::vector<CString> cats = { _T("전체") };
        for (const auto& m : m_vecMenuCache) {
            CString cat = CA2T(m.subCategory.c_str(), CP_UTF8);
            bool found = false;
            for (const auto& c : cats) if (c == cat) { found = true; break; }
            if (!found && !cat.IsEmpty()) cats.push_back(cat);
        }
        m_vecSubCategories = cats;
        if (m_wndScrollMenu.GetSafeHwnd())
            m_wndScrollMenu.SetMenuItems(m_vecSubCategories);

        RebuildMenuListUI(m_vecMenuCache, _T("전체"));
    } else {
        m_listMenu.DeleteAllItems();
        m_listMenu.InsertItem(0, _T("메뉴 정보를 가져오지 못했습니다."));
    }

    delete pBody;
    return 0;
}

// ── UI 갱신 ───────────────────────────────────────────────────
void StoreListDlg::RebuildMenuListUI(const std::vector<MenuInfo>& menus, const CString& filter)
{
    m_listMenu.DeleteAllItems();
    int row = 0;
    for (const auto& m : menus) {
        CString subCat = CA2T(m.subCategory.c_str(), CP_UTF8);
        if (filter != _T("전체") && subCat != filter) continue;

        CString strName  = CA2T(m.menuName.c_str(), CP_UTF8);
        CString strPrice; strPrice.Format(_T("%d원"), m.price);
        int nRow = m_listMenu.InsertItem(row++, strName);
        m_listMenu.SetItemText(nRow, 1, _T(""));  // 설명은 상세 화면에서
        m_listMenu.SetItemText(nRow, 2, strPrice);
    }
    if (row == 0)
        m_listMenu.InsertItem(0, _T("해당 카테고리의 메뉴가 없습니다."));
}

// 기존 OrderManager 기반 로컬 폴백
void StoreListDlg::UpdateMenuListUI(CString subCategory)
{
    m_listMenu.DeleteAllItems();
    m_vecMenuCache.clear();
    std::string sub = CT2A(subCategory, CP_UTF8);
    m_vecMenuCache = OrderManager::GetInstance().GetMenuData(m_storeInfo.storeID, sub);
    RebuildMenuListUI(m_vecMenuCache, subCategory);
}

// ── 서브카테고리 탭 클릭 ─────────────────────────────────────
LRESULT StoreListDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM lParam)
{
    int nIndex = (UINT)wParam - 2000;
    if (nIndex < 0 || nIndex >= (int)m_vecSubCategories.size()) return 0;

    CString selected = m_vecSubCategories[nIndex];

    if (m_vecMenuCache.empty()) {
        // 아직 서버 응답 없음 → 요청 전송
        SendMenuListRequest(selected);
    } else {
        // 이미 캐시 있음 → 클라이언트 필터링
        RebuildMenuListUI(m_vecMenuCache, selected);
    }
    return 0;
}

// ── 메뉴 클릭 → MenuDetailDlg ────────────────────────────────
void StoreListDlg::OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int nIndex = pNMIA->iItem;

    // 리스트의 visible 인덱스가 m_vecMenuCache와 다를 수 있으므로
    // 리스트 첫 번째 열 텍스트로 매칭
    if (nIndex < 0) { *pResult = 0; return; }

    CString strSelected = m_listMenu.GetItemText(nIndex, 0);
    MenuInfo* pFound = nullptr;
    for (auto& m : m_vecMenuCache) {
        CString name = CA2T(m.menuName.c_str(), CP_UTF8);
        if (name == strSelected) { pFound = &m; break; }
    }

    if (pFound) {
        MenuDetailDlg dlg(this);
        dlg.m_strMenuName = CA2T(pFound->menuName.c_str(), CP_UTF8);
        dlg.m_menuInfo    = *pFound;
        dlg.DoModal();
    }
    *pResult = 0;
}

// ── 장바구니 버튼 ─────────────────────────────────────────────
void StoreListDlg::OnBnClickedBtnCart()
{
    CartDlg dlg(this);
    if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
}

// ── 가게 정보 버튼 ────────────────────────────────────────────
void StoreListDlg::OnBnClickedBtnStoreInfo()
{
    StoreDetailDlg dlg(this);
    dlg.m_strName = CA2T(m_storeInfo.storeName.c_str(), CP_UTF8);
    dlg.m_strAddr = CA2T(m_storeInfo.address.c_str(), CP_UTF8);
    dlg.m_strTime = CA2T(m_storeInfo.openTime.c_str(), CP_UTF8);
    dlg.m_strOff  = CA2T(m_storeInfo.holiday.c_str(), CP_UTF8);
    dlg.m_strTel  = CA2T(m_storeInfo.phoneNumber.c_str(), CP_UTF8);
    dlg.DoModal();
}

void StoreListDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MENU_LIST);
    CDialogEx::OnCancel();
}

BOOL StoreListDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    CRect rect;
    m_wndScrollMenu.GetWindowRect(&rect);
    if (rect.PtInRect(pt)) return m_wndScrollMenu.OnMouseWheel(nFlags, zDelta, pt);
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}
