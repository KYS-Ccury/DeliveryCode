// ================================================================
//  MainHomeDlg.cpp  — 이미지 TCP 수신 방식으로 수정
//
//  [변경 사항]
//  1. RebuildStoreListUI: UNC 경로 접근 제거 → 기본 이미지로만 먼저 그림
//  2. RegisterNetworkCallback: REQ_GET_IMAGE(217) 콜백 추가
//  3. RequestStoreImages: 가게 목록 수신 후 이미지 개별 요청
//  4. OnImageResponse: base64 수신 → BitmapFromBytes → ImageList 갱신
//  5. 디버그 MessageBox 제거
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MainHomeDlg.h"
#include "OrderManager.h"
#include "CartDlg.h"
#include "StoreListDlg.h"
#include "OrderHistoryDlg.h"
#include "PaymentDlg.h"
#include "OrderListDlg.h"
#include "MyMenuPopup.h"
#include "PointDlg.h"
#include "EditInfoDlg.h"
#include "MyInfoDlg.h"
#include "ChatDlg.h"
#include "AddressDlg.h"
#include "AddressManager.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "ImageLoader.h"
#include "common/header/Types.h"

#define WM_STORE_LIST_RESPONSE  (WM_USER + 110)
#define WM_ADDR_LIST_RESPONSE   (WM_USER + 115)
#define WM_ADDR_SAVE_RESPONSE   (WM_USER + 116)
// ★ 이미지 응답 메시지
#define WM_IMAGE_RESPONSE       (WM_USER + 117)

static const int STORE_THUMB_W = 60;
static const int STORE_THUMB_H = 60;

// ── 간이 JSON 파싱 ─────────────────────────────────────────────
static std::string MHJStr(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":\"";
    auto p = j.find(t); if (p == std::string::npos) return "";
    p += t.size(); auto e = j.find('"', p);
    return (e == std::string::npos) ? "" : j.substr(p, e - p);
}
static int MHJInt(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return 0;
    try { return std::stoi(j.substr(p + t.size())); } catch (...) { return 0; }
}
static double MHJDouble(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return 0.0;
    try { return std::stod(j.substr(p + t.size())); } catch (...) { return 0.0; }
}
static std::vector<StoreInfo> ParseStoreArray(const std::string& json)
{
    std::vector<StoreInfo> stores;
    auto arrPos = json.find("\"stores\":[");
    if (arrPos == std::string::npos) return stores;
    size_t i = arrPos + 10;
    while (i < json.size()) {
        auto s = json.find('{', i); if (s == std::string::npos) break;
        int d = 0; size_t e = s;
        for (; e < json.size(); ++e) {
            if (json[e]=='{') ++d; else if (json[e]=='}') { if(--d==0) break; }
        }
        std::string o = json.substr(s, e - s + 1);
        StoreInfo si;
        si.storeID            = MHJInt(o,"id");
        si.storeName          = MHJStr(o,"name");
        si.category           = MHJStr(o,"category");
        si.deliveryTime       = MHJStr(o,"delivery_time");
        si.deliveryPriceRange = MHJStr(o,"delivery_fee_str");
        si.address            = MHJStr(o,"address");
        si.openTime           = MHJStr(o,"open_time");
        si.phoneNumber        = MHJStr(o,"phone");
        si.holiday            = MHJStr(o,"holiday");
        si.description        = MHJStr(o,"description");
        si.storeImageUrl      = MHJStr(o,"image_url");
        si.logoUrl            = MHJStr(o,"logo_url");  // ★ 로고
        si.minOrderAmount     = MHJInt(o,"min_order");
        si.distance           = MHJDouble(o,"distance");
        if (si.storeID > 0) stores.push_back(si);
        i = e + 1;
    }
    return stores;
}

IMPLEMENT_DYNAMIC(MainHomeDlg, CDialogEx)

MainHomeDlg::MainHomeDlg(CWnd* pParent)
    : CDialogEx(IDD_MAINHOME_DLG, pParent) {}
MainHomeDlg::~MainHomeDlg() {
    if (m_imgListStore.GetSafeHandle())
        m_imgListStore.DeleteImageList();
}

void MainHomeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_STOR, m_listStore);
}

BEGIN_MESSAGE_MAP(MainHomeDlg, CDialogEx)
    ON_WM_TIMER()
    ON_WM_MOUSEWHEEL()
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED,      &MainHomeDlg::OnScrollMenuClicked)
    ON_MESSAGE(WM_STORE_LIST_RESPONSE,      &MainHomeDlg::OnStoreListResponse)
    ON_MESSAGE(WM_ADDR_LIST_RESPONSE,       &MainHomeDlg::OnAddrListResponse)
    ON_MESSAGE(WM_ADDR_SAVE_RESPONSE,       &MainHomeDlg::OnAddrSaveResponse)
    // ★ 이미지 응답 핸들러
    ON_MESSAGE(WM_IMAGE_RESPONSE,           &MainHomeDlg::OnImageResponse)
    ON_BN_CLICKED(IDC_BUTTON1,              &MainHomeDlg::OnBnClickedButton1)
    ON_NOTIFY(NM_CLICK, IDC_LIST_STOR,      &MainHomeDlg::OnNMDblclkListStor)
    ON_BN_CLICKED(IDC_BTN_MY_MYPAGE,        &MainHomeDlg::OnBnClickedBtnMypage)
    ON_BN_CLICKED(IDC_BTN_MY_PAYMENT,       &MainHomeDlg::OnBnClickedBtnPayment)
    ON_BN_CLICKED(IDC_BTN_MY_DELIVERY,      &MainHomeDlg::OnBnClickedBtnDelivery)
    ON_BN_CLICKED(IDC_BTN_MY_ORDERHISTORY,  &MainHomeDlg::OnBnClickedBtnOrderHistory)
    ON_BN_CLICKED(IDC_BTN_ADDR_LABEL,       &MainHomeDlg::OnBnClickedBtnAddr)
    ON_MESSAGE(WM_MYMENU_SELECTED,          &MainHomeDlg::OnMyMenuSelected)
END_MESSAGE_MAP()

BOOL MainHomeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN | WS_DLGFRAME);
    CenterWindow();

    m_brushBack.CreateSolidBrush(RGB(230, 245, 245));
    m_brushWhite.CreateSolidBrush(RGB(255, 255, 255));

    m_imgListStore.Create(STORE_THUMB_W, STORE_THUMB_H, ILC_COLOR32, 16, 8);
    m_listStore.SetImageList(&m_imgListStore, LVSIL_SMALL);

    m_listStore.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listStore.InsertColumn(0, _T("사진"),     LVCFMT_LEFT,   70);
    m_listStore.InsertColumn(1, _T("매장명"),   LVCFMT_LEFT,  140);
    m_listStore.InsertColumn(2, _T("배달시간"), LVCFMT_CENTER,  70);
    m_listStore.InsertColumn(3, _T("배달비"),   LVCFMT_CENTER,  70);
    m_listStore.InsertColumn(4, _T("최소주문"), LVCFMT_RIGHT,   85);
    m_listStore.InsertColumn(5, _T("가게소개"), LVCFMT_LEFT,   190);

    CWnd* pPH = GetDlgItem(IDC_STATIC_MENU_BAR);
    if (pPH) {
        CRect rect; pPH->GetWindowRect(&rect); ScreenToClient(&rect);
        pPH->ShowWindow(SW_HIDE);
        m_wndScrollMenu.Create(_T(""), WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|SS_NOTIFY,
                               rect, this, IDC_STATIC_MENU_BAR);
    }
    m_vecCategories = { _T("전체"),_T("족발/보쌈"),_T("찜/탕"),_T("일식"),
                        _T("치킨"),_T("피자"),_T("중식"),_T("양식") };
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecCategories);

    m_bLastConnState = NetworkManager::GetInstance().IsConnected();
    UpdateConnStatusUI();
    SetTimer(TIMER_CONN_CHECK, 3000, nullptr);

    RegisterNetworkCallback();
    UpdateAddrLabel();
    SendStoreListRequest(_T("전체"));
    return TRUE;
}

void MainHomeDlg::UpdateConnStatusUI()
{
    CWnd* pLabel = this->GetDlgItem(IDC_STATIC_CONN_STATUS);
    if (!pLabel) return;
    bool bConn = NetworkManager::GetInstance().IsConnected();
    pLabel->SetWindowText(bConn ? _T("● 서버 연결됨") : _T("● 서버 연결 안됨"));
    if (bConn != m_bLastConnState) {
        m_bLastConnState = bConn;
        pLabel->Invalidate();
    }
}

void MainHomeDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_CONN_CHECK) {
        bool bConn = NetworkManager::GetInstance().IsConnected();
        if (bConn != m_bLastConnState) {
            UpdateConnStatusUI();
            if (!bConn) {
                m_listStore.DeleteAllItems();
                m_listStore.InsertItem(0, _T("서버 연결이 끊겼습니다."));
            }
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

HBRUSH MainHomeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC && pWnd &&
        pWnd->GetDlgCtrlID() == IDC_STATIC_CONN_STATUS)
    {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(NetworkManager::GetInstance().IsConnected()
                          ? RGB(0,160,0) : RGB(200,0,0));
        return m_brushBack;
    }
    if (nCtlColor == CTLCOLOR_DLG)    return m_brushBack;
    if (nCtlColor == CTLCOLOR_STATIC) { pDC->SetBkMode(TRANSPARENT); return m_brushWhite; }
    return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

// ================================================================
//  RegisterNetworkCallback — ★ 이미지 응답 콜백 추가
// ================================================================
void MainHomeDlg::RegisterNetworkCallback()
{
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_STORE_LIST,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_STORE_LIST_RESPONSE, 0, (LPARAM)p);
        });

    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_GET_ADDRESSES,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_ADDR_LIST_RESPONSE, 0, (LPARAM)p);
        });

    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_SAVE_ADDRESS,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_ADDR_SAVE_RESPONSE, 0, (LPARAM)p);
        });

    // ★ 이미지 응답 콜백
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_GET_IMAGE,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_IMAGE_RESPONSE, 0, (LPARAM)p);
        });
}

void MainHomeDlg::SendStoreListRequest(const CString& category)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        OrderManager::GetInstance().LoadStoreData();
        UpdateStoreListUI(category); return;
    }
    std::string cat = (category==_T("전체")) ? "" : std::string(CT2A(category,CP_UTF8));
    std::string json = "{\"category\":\"" + cat + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_STORE_LIST, json);
    m_listStore.DeleteAllItems();
    m_listStore.InsertItem(0, _T("불러오는 중..."));
}

LRESULT MainHomeDlg::OnStoreListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;
    if (MHJInt(*pBody,"status") == (int)Status::SUCCESS) {
        m_vecStoreCache = ParseStoreArray(*pBody);
        OrderManager::GetInstance().UpdateStoreCache(m_vecStoreCache);
        // ★ 1단계: 기본 이미지로 먼저 리스트 표시
        RebuildStoreListUI(m_vecStoreCache);
        // ★ 2단계: 이미지 비동기 요청
        RequestStoreImages(m_vecStoreCache);
    } else {
        m_listStore.DeleteAllItems();
        m_listStore.InsertItem(0, _T("가게 정보를 가져오지 못했습니다."));
    }
    delete pBody; return 0;
}

// ================================================================
//  RebuildStoreListUI
//  ★ UNC 경로 접근 제거 — 기본 회색 이미지로만 먼저 그림
//  ★ 디버그 MessageBox 제거
// ================================================================
void MainHomeDlg::RebuildStoreListUI(const std::vector<StoreInfo>& stores)
{
    m_listStore.DeleteAllItems();

    if (m_imgListStore.GetSafeHandle())
        m_imgListStore.DeleteImageList();
    m_imgListStore.Create(STORE_THUMB_W, STORE_THUMB_H, ILC_COLOR32,
                          (int)stores.size() + 1, 4);
    m_listStore.SetImageList(&m_imgListStore, LVSIL_SMALL);

    // 기본 이미지 (회색 박스) — index 0
    {
        HDC hdcScreen = ::GetDC(nullptr);
        HDC hdc = ::CreateCompatibleDC(hdcScreen);
        HBITMAP hDef = ::CreateCompatibleBitmap(hdcScreen, STORE_THUMB_W, STORE_THUMB_H);
        HGDIOBJ hOld = ::SelectObject(hdc, hDef);
        RECT rc = {0, 0, STORE_THUMB_W, STORE_THUMB_H};
        HBRUSH hBr = ::CreateSolidBrush(RGB(210, 210, 210));
        ::FillRect(hdc, &rc, hBr);
        ::DeleteObject(hBr);
        ::SelectObject(hdc, hOld);
        ::DeleteDC(hdc);
        ::ReleaseDC(nullptr, hdcScreen);
        m_imgListStore.Add(CBitmap::FromHandle(hDef), (CBitmap*)nullptr);
        ::DeleteObject(hDef);
    }
    // index 1 ~ N: 각 가게 슬롯 (기본 이미지로 채움, 나중에 OnImageResponse에서 교체)
    for (int i = 0; i < (int)stores.size(); ++i) {
        // 빈 슬롯 추가 (기본 이미지 복사)
        HDC hdcScreen = ::GetDC(nullptr);
        HDC hdc = ::CreateCompatibleDC(hdcScreen);
        HBITMAP hSlot = ::CreateCompatibleBitmap(hdcScreen, STORE_THUMB_W, STORE_THUMB_H);
        HGDIOBJ hOld = ::SelectObject(hdc, hSlot);
        RECT rc = {0, 0, STORE_THUMB_W, STORE_THUMB_H};
        HBRUSH hBr = ::CreateSolidBrush(RGB(210, 210, 210));
        ::FillRect(hdc, &rc, hBr);
        ::DeleteObject(hBr);
        ::SelectObject(hdc, hOld);
        ::DeleteDC(hdc);
        ::ReleaseDC(nullptr, hdcScreen);
        m_imgListStore.Add(CBitmap::FromHandle(hSlot), (CBitmap*)nullptr);
        ::DeleteObject(hSlot);

        // 리스트 아이템 삽입 (이미지 인덱스 = i+1)
        LVITEM lvi = {};
        lvi.mask     = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem    = i;
        lvi.iSubItem = 0;
        lvi.pszText  = (LPTSTR)(LPCTSTR)_T("");
        lvi.iImage   = i + 1; // 각 가게 전용 슬롯
        int r = m_listStore.InsertItem(&lvi);

        CString n = CA2T(stores[i].storeName.c_str(), CP_UTF8);
        m_listStore.SetItemText(r, 1, n);
        CString t = CA2T(stores[i].deliveryTime.c_str(), CP_UTF8);
        m_listStore.SetItemText(r, 2, t.IsEmpty() ? _T("--") : t);
        CString f = CA2T(stores[i].deliveryPriceRange.c_str(), CP_UTF8);
        m_listStore.SetItemText(r, 3, f.IsEmpty() ? _T("--") : f);
        CString a; a.Format(_T("%d원"), stores[i].minOrderAmount);
        m_listStore.SetItemText(r, 4, a);
        CString desc = CA2T(stores[i].description.c_str(), CP_UTF8);
        m_listStore.SetItemText(r, 5, desc);
    }
    if (stores.empty())
        m_listStore.InsertItem(0, _T("해당 카테고리의 가게가 없습니다."));
}

// ================================================================
//  RequestStoreImages  ★ 신규
//  가게 목록 표시 후 image_url이 있는 가게의 이미지를 순차 요청
//  m_imageUrlToIndex: image_url → ImageList 슬롯 인덱스 맵핑
// ================================================================
void MainHomeDlg::RequestStoreImages(const std::vector<StoreInfo>& stores)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    m_imageUrlToIndex.clear();

    std::string token = AuthManager::GetInstance().GetAccessToken();

    for (int i = 0; i < (int)stores.size(); ++i) {
        // ★ 로고 우선, 없으면 메뉴 이미지 사용
        const std::string& url = !stores[i].logoUrl.empty()
                                 ? stores[i].logoUrl
                                 : stores[i].storeImageUrl;
        if (url.empty()) continue;
        // ImageList 슬롯 인덱스 = i+1 (0은 기본 이미지)
        m_imageUrlToIndex[url] = i + 1;

        // 서버에 이미지 요청 (REQ_GET_IMAGE = 217)
        std::string json = "{\"token\":\"" + token + "\","
                           "\"image_url\":\"" + url + "\"}";
        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCustomer::REQ_GET_IMAGE, json);
    }
}

// ================================================================
//  OnImageResponse  ★ 신규  (WM_IMAGE_RESPONSE)
//  서버 응답: { "status":2000, "image_url":"...", "data":"<base64>" }
//  → base64 디코딩 → BitmapFromBytes → ImageList 슬롯 교체 → 행 갱신
// ================================================================
LRESULT MainHomeDlg::OnImageResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = MHJInt(*pBody, "status");
    std::string imageUrl = MHJStr(*pBody, "image_url");
    std::string b64Data  = MHJStr(*pBody, "data");

    delete pBody;

    if (status != (int)Status::SUCCESS || b64Data.empty() || imageUrl.empty())
        return 0;

    // image_url → ImageList 슬롯 인덱스 조회
    auto it = m_imageUrlToIndex.find(imageUrl);
    if (it == m_imageUrlToIndex.end()) return 0;
    int imgSlot = it->second;

    // base64 → 바이트 → HBITMAP
    auto bytes = ImageLoader::DecodeBase64(b64Data);
    HBITMAP hBmp = ImageLoader::BitmapFromBytes(bytes, STORE_THUMB_W, STORE_THUMB_H);
    if (!hBmp) return 0;

    // ImageList 슬롯 교체
    m_imgListStore.Replace(imgSlot, CBitmap::FromHandle(hBmp), (CBitmap*)nullptr);
    ::DeleteObject(hBmp);

    // 해당 슬롯을 사용하는 행 찾아서 갱신 (RedrawItems)
    int count = m_listStore.GetItemCount();
    for (int i = 0; i < count; ++i) {
        LVITEM lvi = {};
        lvi.mask    = LVIF_IMAGE;
        lvi.iItem   = i;
        lvi.iSubItem = 0;
        m_listStore.GetItem(&lvi);
        if (lvi.iImage == imgSlot) {
            m_listStore.RedrawItems(i, i);
            break;
        }
    }
    m_listStore.UpdateWindow();
    return 0;
}

LRESULT MainHomeDlg::OnAddrListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;
    if (MHJInt(*pBody, "status") == (int)Status::SUCCESS) {
        AddressManager::GetInstance().OnAddressListResponse(*pBody);
        UpdateAddrLabel();
    }
    delete pBody; return 0;
}

LRESULT MainHomeDlg::OnAddrSaveResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;
    if (MHJInt(*pBody, "status") == (int)Status::SUCCESS)
        AddressManager::GetInstance().OnSaveAddressResponse(*pBody);
    delete pBody; return 0;
}

void MainHomeDlg::UpdateStoreListUI(CString cat)
{
    m_listStore.DeleteAllItems(); m_vecStoreCache.clear();
    std::string catStr = CT2A(cat, CP_UTF8);
    auto stores = OrderManager::GetInstance().GetStoresByCategory(catStr);
    RebuildStoreListUI(stores); m_vecStoreCache = stores;
}

LRESULT MainHomeDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM)
{
    int n = (UINT)wParam - SCROLL_MENU_BTN_ID;
    if (n >= 0 && n < (int)m_vecCategories.size())
        SendStoreListRequest(m_vecCategories[n]);
    return 0;
}

void MainHomeDlg::OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult)
{
    int n = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR)->iItem;
    if (n >= 0 && n < (int)m_vecStoreCache.size()) {
        OrderManager::GetInstance().SelectStore(m_vecStoreCache[n].storeID);
        StoreListDlg dlg(this);
        dlg.m_strStoreName = CA2T(m_vecStoreCache[n].storeName.c_str(),CP_UTF8);
        dlg.m_storeInfo    = m_vecStoreCache[n];  // logoUrl 포함됨
        dlg.DoModal();
    }
    *pResult = 0;
}

void MainHomeDlg::OnBnClickedButton1() { CartDlg dlg(this); dlg.DoModal(); }

void MainHomeDlg::OnBnClickedBtnMypage()
{
    CWnd* pBtn = GetDlgItem(IDC_BTN_MY_MYPAGE);
    if (!pBtn) return;
    CRect rcBtn; pBtn->GetWindowRect(&rcBtn);
    if (m_pMyMenuPopup && ::IsWindow(m_pMyMenuPopup->GetSafeHwnd())) return;
    m_pMyMenuPopup = new MyMenuPopup(this);
    m_pMyMenuPopup->ShowAt(CPoint(rcBtn.left, rcBtn.top));
}
void MainHomeDlg::OnBnClickedBtnPayment()      { PaymentDlg dlg(this); dlg.DoModal(); }
void MainHomeDlg::OnBnClickedBtnDelivery()     { OrderHistoryDlg dlg(this); dlg.DoModal(); }
void MainHomeDlg::OnBnClickedBtnOrderHistory() { OrderListDlg dlg(this); dlg.DoModal(); }

void MainHomeDlg::OnCancel()
{
    KillTimer(TIMER_CONN_CHECK);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_ADDRESSES);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_SAVE_ADDRESS);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_IMAGE);
    AddressManager::GetInstance().Clear();
    m_imageUrlToIndex.clear();
    CDialogEx::OnCancel();
}

BOOL MainHomeDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (m_wndScrollMenu.GetSafeHwnd()) {
        CRect r; m_wndScrollMenu.GetWindowRect(&r);
        if (r.PtInRect(pt)) return m_wndScrollMenu.OnMouseWheel(nFlags,zDelta,pt);
    }
    return CDialogEx::OnMouseWheel(nFlags,zDelta,pt);
}

LRESULT MainHomeDlg::OnMyMenuSelected(WPARAM wParam, LPARAM)
{
    m_pMyMenuPopup = nullptr;
    int id = (int)wParam;
    switch (id) {
    case MYMENU_MY_INFO:   { MyInfoDlg dlg(this); dlg.DoModal(); break; }
    case MYMENU_EDIT_INFO: { EditInfoDlg dlg(this); dlg.DoModal(); break; }
    case MYMENU_POINT:     { PointDlg dlg(this); dlg.DoModal(); break; }
    case MYMENU_ADMIN_CHAT: {
        ChatDlg dlg(this);
        dlg.m_strTargetName = _T("관리자 문의");
        dlg.m_strTargetID   = _T("admin");
        dlg.m_strTargetType = _T("admin");
        dlg.DoModal(); break;
    }
    case MYMENU_LOGOUT: {
        CString strUserID = CA2T(AuthManager::GetInstance().GetCurrentUserID().c_str(), CP_UTF8);
        CString msg; msg.Format(_T("로그인 계정: %s\n\n로그아웃 하시겠습니까?"), (LPCTSTR)strUserID);
        if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES) {
            KillTimer(TIMER_CONN_CHECK);
            NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_ADDRESSES);
            NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_SAVE_ADDRESS);
            NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_IMAGE);
            AddressManager::GetInstance().Clear();
            m_imageUrlToIndex.clear();
            AuthManager::GetInstance().Logout();
            CDialogEx::OnCancel();
        }
        break;
    }
    }
    return 0;
}

void MainHomeDlg::OnBnClickedBtnAddr()
{
    AddressDlg dlg(this);
    dlg.DoModal();
    UpdateAddrLabel();
}

void MainHomeDlg::UpdateAddrLabel()
{
    std::string def = AddressManager::GetInstance().GetDefaultAddress();
    CString label = def.empty() ? _T("📍 내 주소")
                                : (_T("📍 ") + CString(CA2T(def.c_str(), CP_UTF8)));
    CWnd* pBtn = GetDlgItem(IDC_BTN_ADDR_LABEL);
    if (pBtn) pBtn->SetWindowText(label);
}

// LoadImageFromServer, ResizeBitmapTo — 하위 호환용 (더 이상 사용 안 함)
HBITMAP MainHomeDlg::LoadImageFromServer(const CString& relPath)
{
    CString fullPath = ImageLoader::MakeServerPath(relPath);
    return ImageLoader::Load(fullPath);
}
void MainHomeDlg::ResizeBitmapTo(HBITMAP& hBmp, int w, int h)
{
    if (!hBmp) return;
    BITMAP bm = {}; ::GetObject(hBmp, sizeof(bm), &bm);
    if (bm.bmWidth == w && bm.bmHeight == h) return;
    HDC s = ::CreateCompatibleDC(nullptr), d = ::CreateCompatibleDC(nullptr);
    HBITMAP hN = ::CreateCompatibleBitmap(s, w, h);
    auto os = ::SelectObject(s, hBmp), od = ::SelectObject(d, hN);
    ::SetStretchBltMode(d, HALFTONE);
    ::StretchBlt(d, 0, 0, w, h, s, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    ::SelectObject(s, os); ::SelectObject(d, od);
    ::DeleteDC(s); ::DeleteDC(d); ::DeleteObject(hBmp); hBmp = hN;
}
