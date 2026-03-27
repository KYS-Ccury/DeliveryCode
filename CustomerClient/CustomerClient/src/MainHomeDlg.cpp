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
// ★ 주소 관련 응답 메시지 (MainHomeDlg.h 와 일치해야 함)
#define WM_ADDR_LIST_RESPONSE   (WM_USER + 115)
#define WM_ADDR_SAVE_RESPONSE   (WM_USER + 116)

// 가게 목록 썸네일 크기
static const int STORE_THUMB_W = 60;
static const int STORE_THUMB_H = 60;

// 서버 이미지 공유 경로 루트 (UNC)
static const TCHAR* SERVER_IMAGE_ROOT = _T("\\\\10.10.10.122\\images\\");

// ── 간이 JSON 파싱 ────────────────────────────────────────────
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
    // ★ 주소 응답 메시지 핸들러 등록
    ON_MESSAGE(WM_ADDR_LIST_RESPONSE,       &MainHomeDlg::OnAddrListResponse)
    ON_MESSAGE(WM_ADDR_SAVE_RESPONSE,       &MainHomeDlg::OnAddrSaveResponse)
    ON_BN_CLICKED(IDC_BUTTON1,              &MainHomeDlg::OnBnClickedButton1)
    ON_NOTIFY(NM_CLICK, IDC_LIST_STOR,      &MainHomeDlg::OnNMDblclkListStor)
    ON_BN_CLICKED(IDC_BTN_MY_MYPAGE,        &MainHomeDlg::OnBnClickedBtnMypage)
    ON_BN_CLICKED(IDC_BTN_MY_PAYMENT,       &MainHomeDlg::OnBnClickedBtnPayment)
    ON_BN_CLICKED(IDC_BTN_MY_DELIVERY,      &MainHomeDlg::OnBnClickedBtnDelivery)
    ON_BN_CLICKED(IDC_BTN_MY_ORDERHISTORY,  &MainHomeDlg::OnBnClickedBtnOrderHistory)
    ON_BN_CLICKED(IDC_BTN_ADDR_LABEL,       &MainHomeDlg::OnBnClickedBtnAddr)
    ON_MESSAGE(WM_MYMENU_SELECTED, &MainHomeDlg::OnMyMenuSelected)
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

    // ★ 콜백 등록 (주소 응답 콜백 포함)
    RegisterNetworkCallback();

    // ★ 로그인 직후 주소 레이블 초기 표시
    //   RequestAddressesFromServer()는 LoginDlg에서 이미 호출됨
    //   서버 응답이 오면 OnAddrListResponse → UpdateAddrLabel 갱신됨
    UpdateAddrLabel();

    SendStoreListRequest(_T("전체"));
    return TRUE;
}

// ── 서버 UNC 경로로 이미지 로드 ──────────────────────────────
HBITMAP MainHomeDlg::LoadImageFromServer(const CString& relPath)
{
    CString fullPath = ImageLoader::MakeServerPath(relPath);
    return ImageLoader::Load(fullPath);
}

void MainHomeDlg::ResizeBitmapTo(HBITMAP& hBmp, int w, int h)
{
    if (!hBmp) return;
    BITMAP bm = {};
    ::GetObject(hBmp, sizeof(bm), &bm);
    if (bm.bmWidth == w && bm.bmHeight == h) return;

    HDC hdcSrc = ::CreateCompatibleDC(nullptr);
    HDC hdcDst = ::CreateCompatibleDC(nullptr);
    HBITMAP hNew = ::CreateCompatibleBitmap(hdcSrc, w, h);

    HGDIOBJ hOldSrc = ::SelectObject(hdcSrc, hBmp);
    HGDIOBJ hOldDst = ::SelectObject(hdcDst, hNew);

    ::SetStretchBltMode(hdcDst, HALFTONE);
    ::StretchBlt(hdcDst, 0, 0, w, h, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    ::SelectObject(hdcSrc, hOldSrc);
    ::SelectObject(hdcDst, hOldDst);
    ::DeleteDC(hdcSrc);
    ::DeleteDC(hdcDst);
    ::DeleteObject(hBmp);
    hBmp = hNew;
}

// ── 연결 상태 UI ─────────────────────────────────────────────
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
//  RegisterNetworkCallback
//  ★ 주소 관련 콜백 2개 추가
// ================================================================
void MainHomeDlg::RegisterNetworkCallback()
{
    // 가게 목록 응답 콜백
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_STORE_LIST,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            PostMessage(WM_STORE_LIST_RESPONSE, 0, (LPARAM)pBody);
        });

    // ★ 주소 목록 조회 응답 콜백 (213)
    //   LoginDlg에서 RequestAddressesFromServer()를 호출하면
    //   서버가 213으로 응답 → 여기서 수신 → OnAddrListResponse 처리
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_GET_ADDRESSES,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            PostMessage(WM_ADDR_LIST_RESPONSE, 0, (LPARAM)pBody);
        });

    // ★ 주소 저장 응답 콜백 (214)
    //   AddressManager::AddAddress() 내부에서 패킷 전송 →
    //   서버가 214로 address_id 포함 응답 → OnAddrSaveResponse 처리
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_SAVE_ADDRESS,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            PostMessage(WM_ADDR_SAVE_RESPONSE, 0, (LPARAM)pBody);
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
        RebuildStoreListUI(m_vecStoreCache);
    } else {
        m_listStore.DeleteAllItems();
        m_listStore.InsertItem(0, _T("가게 정보를 가져오지 못했습니다."));
    }
    delete pBody; return 0;
}

// ================================================================
//  ★ OnAddrListResponse  —  REQ_GET_ADDRESSES (213) 응답 처리
//
//  서버 응답 예시:
//  { "status":2000,
//    "addresses":[
//      {"address_id":1,"address":"광주시 북구 용봉동","label":"집","is_default":true},
//      {"address_id":2,"address":"광주시 서구 치평동","label":"회사","is_default":false}
//    ] }
//
//  처리 흐름:
//    1. AddressManager::OnAddressListResponse() → m_list 재구성
//    2. UpdateAddrLabel() → 상단 주소 버튼 텍스트 갱신
// ================================================================
LRESULT MainHomeDlg::OnAddrListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (MHJInt(*pBody, "status") == (int)Status::SUCCESS) {
        // AddressManager에 서버 주소 목록 반영
        AddressManager::GetInstance().OnAddressListResponse(*pBody);
        // 상단 주소 버튼 레이블 갱신
        UpdateAddrLabel();
    }

    delete pBody;
    return 0;
}

// ================================================================
//  ★ OnAddrSaveResponse  —  REQ_SAVE_ADDRESS (214) 응답 처리
//
//  서버 응답 예시:
//  { "status":2000, "address_id":5 }
//
//  처리 흐름:
//    1. AddressManager::OnSaveAddressResponse() →
//       addressId==0 인 항목(방금 추가된 것)에 서버 발급 ID 저장
//    2. 이후 삭제/기본설정 시 address_id를 서버에 정확히 전달 가능
// ================================================================
LRESULT MainHomeDlg::OnAddrSaveResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (MHJInt(*pBody, "status") == (int)Status::SUCCESS) {
        AddressManager::GetInstance().OnSaveAddressResponse(*pBody);
    }

    delete pBody;
    return 0;
}

void MainHomeDlg::RebuildStoreListUI(const std::vector<StoreInfo>& stores)
{
    m_listStore.DeleteAllItems();

    if (m_imgListStore.GetSafeHandle())
        m_imgListStore.DeleteImageList();
    m_imgListStore.Create(STORE_THUMB_W, STORE_THUMB_H, ILC_COLOR32, (int)stores.size() + 1, 4);
    m_listStore.SetImageList(&m_imgListStore, LVSIL_SMALL);

    HBITMAP hDefault = nullptr;
    {
        HDC hdcScreen = ::GetDC(nullptr);
        HDC hdc = ::CreateCompatibleDC(hdcScreen);
        hDefault = ::CreateCompatibleBitmap(hdcScreen, STORE_THUMB_W, STORE_THUMB_H);
        HGDIOBJ hOld = ::SelectObject(hdc, hDefault);
        RECT rc = {0, 0, STORE_THUMB_W, STORE_THUMB_H};
        HBRUSH hBr = ::CreateSolidBrush(RGB(210, 210, 210));
        ::FillRect(hdc, &rc, hBr);
        ::DeleteObject(hBr);
        ::SelectObject(hdc, hOld);
        ::DeleteDC(hdc);
        ::ReleaseDC(nullptr, hdcScreen);
    }
    int defImgIdx = m_imgListStore.Add(CBitmap::FromHandle(hDefault), (CBitmap*)nullptr);
    ::DeleteObject(hDefault);

    for (int i = 0; i < (int)stores.size(); ++i) {
        int imgIdx = defImgIdx;
        CString imgUrl = CA2T(stores[i].storeImageUrl.c_str(), CP_UTF8);
        if (!imgUrl.IsEmpty()) {
            CString fullPath = ImageLoader::MakeServerPath(imgUrl);
            HBITMAP hBmp = ImageLoader::LoadResized(fullPath, STORE_THUMB_W, STORE_THUMB_H);
            if (hBmp) {
                imgIdx = m_imgListStore.Add(CBitmap::FromHandle(hBmp), (CBitmap*)nullptr);
                ::DeleteObject(hBmp);
            }
        }

        CString n = CA2T(stores[i].storeName.c_str(), CP_UTF8);
        LVITEM lvi = {};
        lvi.mask    = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem   = i;
        lvi.iSubItem= 0;
        lvi.pszText = (LPTSTR)(LPCTSTR)_T("");
        lvi.iImage  = imgIdx;
        int r = m_listStore.InsertItem(&lvi);

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
    if (stores.empty()) m_listStore.InsertItem(0,_T("해당 카테고리의 가게가 없습니다."));
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
        dlg.m_storeInfo    = m_vecStoreCache[n];
        dlg.DoModal();
    }
    *pResult = 0;
}

void MainHomeDlg::OnBnClickedButton1()
{
    CartDlg dlg(this); dlg.DoModal();
}

// ── 하단 4버튼 핸들러 ─────────────────────────────────────────
void MainHomeDlg::OnBnClickedBtnMypage()
{
    CWnd* pBtn = GetDlgItem(IDC_BTN_MY_MYPAGE);
    if (!pBtn) return;
    CRect rcBtn; pBtn->GetWindowRect(&rcBtn);
    CPoint ptPopup(rcBtn.left, rcBtn.top);
    if (m_pMyMenuPopup && ::IsWindow(m_pMyMenuPopup->GetSafeHwnd())) return;
    m_pMyMenuPopup = new MyMenuPopup(this);
    m_pMyMenuPopup->ShowAt(ptPopup);
}
void MainHomeDlg::OnBnClickedBtnPayment()  { PaymentDlg dlg(this); dlg.DoModal(); }
void MainHomeDlg::OnBnClickedBtnDelivery() { OrderHistoryDlg dlg(this); dlg.DoModal(); }
void MainHomeDlg::OnBnClickedBtnOrderHistory() { OrderListDlg dlg(this); dlg.DoModal(); }

void MainHomeDlg::OnCancel()
{
    KillTimer(TIMER_CONN_CHECK);
    // ★ 로그아웃 시 주소 콜백 해제 + 메모리 초기화
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_ADDRESSES);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_SAVE_ADDRESS);
    AddressManager::GetInstance().Clear();
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
            // ★ 로그아웃 시 주소 데이터 초기화 및 콜백 해제
            NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_ADDRESSES);
            NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_SAVE_ADDRESS);
            AddressManager::GetInstance().Clear();
            AuthManager::GetInstance().Logout();
            CDialogEx::OnCancel();
        }
        break;
    }
    }
    return 0;
}

// ================================================================
//  IDC_BTN_ADDR_LABEL 클릭 — 주소 관리 다이얼로그 열기
// ================================================================
void MainHomeDlg::OnBnClickedBtnAddr()
{
    AddressDlg dlg(this);
    dlg.DoModal();
    // ★ 다이얼로그 닫힌 후 레이블 갱신
    //   (AddressDlg 내에서 주소 추가/삭제/선택이 일어났을 수 있음)
    UpdateAddrLabel();
}

// ── 기본 주소를 상단 버튼에 반영 ─────────────────────────────
void MainHomeDlg::UpdateAddrLabel()
{
    std::string def = AddressManager::GetInstance().GetDefaultAddress();
    CString label;
    if (def.empty()) {
        label = _T("📍 내 주소");
    } else {
        CString strDef = CA2T(def.c_str(), CP_UTF8);
        label = _T("📍 ") + strDef;
    }

    CWnd* pBtn = GetDlgItem(IDC_BTN_ADDR_LABEL);
    if (pBtn) pBtn->SetWindowText(label);
}
