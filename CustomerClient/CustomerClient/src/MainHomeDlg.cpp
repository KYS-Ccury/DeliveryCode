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
#include "AuthManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

#define WM_STORE_LIST_RESPONSE (WM_USER + 110)

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
        si.storeID           = MHJInt(o,"id");          // 서버가 "id" 로 전송
        si.storeName         = MHJStr(o,"name");
        si.category          = MHJStr(o,"category");
        si.deliveryTime      = MHJStr(o,"delivery_time");
        si.deliveryPriceRange= MHJStr(o,"delivery_fee");
        si.address           = MHJStr(o,"address");
        si.openTime          = MHJStr(o,"open_time");
        si.phoneNumber       = MHJStr(o,"phone");
        si.holiday           = MHJStr(o,"holiday");
        si.minOrderAmount    = MHJInt(o,"min_order");
        si.distance          = MHJDouble(o,"distance");
        if (si.storeID > 0) stores.push_back(si);
        i = e + 1;
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
    ON_WM_TIMER()
    ON_WM_MOUSEWHEEL()
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED,      &MainHomeDlg::OnScrollMenuClicked)
    ON_MESSAGE(WM_STORE_LIST_RESPONSE,      &MainHomeDlg::OnStoreListResponse)
    ON_BN_CLICKED(IDC_BUTTON1,              &MainHomeDlg::OnBnClickedButton1)
    ON_NOTIFY(NM_CLICK, IDC_LIST_STOR,      &MainHomeDlg::OnNMDblclkListStor)
    ON_BN_CLICKED(IDC_BTN_MY_MYPAGE,        &MainHomeDlg::OnBnClickedBtnMypage)
    ON_BN_CLICKED(IDC_BTN_MY_PAYMENT,       &MainHomeDlg::OnBnClickedBtnPayment)
    ON_BN_CLICKED(IDC_BTN_MY_DELIVERY,      &MainHomeDlg::OnBnClickedBtnDelivery)
    ON_BN_CLICKED(IDC_BTN_MY_ORDERHISTORY,  &MainHomeDlg::OnBnClickedBtnOrderHistory)
    ON_MESSAGE(WM_MYMENU_SELECTED, &MainHomeDlg::OnMyMenuSelected)
END_MESSAGE_MAP()

BOOL MainHomeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN | WS_DLGFRAME);
    CenterWindow();

    m_brushBack.CreateSolidBrush(RGB(230, 245, 245));
    m_brushWhite.CreateSolidBrush(RGB(255, 255, 255));

    m_listStore.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listStore.InsertColumn(0, _T("매장명"),       LVCFMT_LEFT,   170);
    m_listStore.InsertColumn(1, _T("배달시간"),     LVCFMT_CENTER,  80);
    m_listStore.InsertColumn(2, _T("최소주문"),     LVCFMT_RIGHT,  100);
    m_listStore.InsertColumn(3, _T("거리"),         LVCFMT_CENTER,  55);

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
    SendStoreListRequest(_T("전체"));
    return TRUE;
}

// ── 연결 상태 UI ─────────────────────────────────────────────
void MainHomeDlg::UpdateConnStatusUI()
{
    CWnd* pLabel = this->GetDlgItem(IDC_STATIC_CONN_STATUS);
    if (!pLabel) return;
    bool bConn = NetworkManager::GetInstance().IsConnected();
    if (bConn)
        pLabel->SetWindowText(_T("● 서버 연결됨"));
    else
        pLabel->SetWindowText(_T("● 서버 연결 안됨"));
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

void MainHomeDlg::RegisterNetworkCallback()
{
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_STORE_LIST,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            PostMessage(WM_STORE_LIST_RESPONSE, 0, (LPARAM)pBody);
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
void MainHomeDlg::RebuildStoreListUI(const std::vector<StoreInfo>& stores)
{
    m_listStore.DeleteAllItems();
    for (int i = 0; i < (int)stores.size(); ++i) {
        CString n=CA2T(stores[i].storeName.c_str(),CP_UTF8);
        CString t=CA2T(stores[i].deliveryTime.c_str(),CP_UTF8);
        int r = m_listStore.InsertItem(i, n);
        m_listStore.SetItemText(r,1,t);
        CString a; a.Format(_T("%d원"),stores[i].minOrderAmount);
        m_listStore.SetItemText(r,2,a);
        CString d; d.Format(_T("%.1fkm"),stores[i].distance);
        m_listStore.SetItemText(r,3,d);
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
    //// 로그인 유저 정보 팝업 (간단 알림 + 로그아웃)
    //CString strUserID = CA2T(AuthManager::GetInstance().GetCurrentUserID().c_str(), CP_UTF8);
    //CString msg;
    //msg.Format(_T("로그인 계정: %s\n\n로그아웃 하시겠습니까?"), (LPCTSTR)strUserID);
    //if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES) {
    //    KillTimer(TIMER_CONN_CHECK);
    //    AuthManager::GetInstance().Logout();
    //    CDialogEx::OnCancel();
    //}

    // My 버튼 위치 기준으로 팝업 표시
    CWnd* pBtn = GetDlgItem(IDC_BTN_MY_MYPAGE);
    if (!pBtn) return;

    CRect rcBtn;
    pBtn->GetWindowRect(&rcBtn);

    // 팝업은 버튼 위쪽에 표시 (하단 버튼이므로)
    CPoint ptPopup(rcBtn.left, rcBtn.top);

    if (m_pMyMenuPopup && ::IsWindow(m_pMyMenuPopup->GetSafeHwnd()))
        return;  // 이미 열려있으면 무시

    m_pMyMenuPopup = new MyMenuPopup(this);
    m_pMyMenuPopup->ShowAt(ptPopup);
}

void MainHomeDlg::OnBnClickedBtnPayment()
{
    PaymentDlg dlg(this); dlg.DoModal();
}
void MainHomeDlg::OnBnClickedBtnDelivery()
{
    // 진행 중인 최근 주문 현황
    OrderHistoryDlg dlg(this);
    dlg.DoModal();
}
void MainHomeDlg::OnBnClickedBtnOrderHistory()
{
    // 전체 주문 내역
    OrderListDlg dlg(this);
    dlg.DoModal();
}

void MainHomeDlg::OnCancel()
{
    KillTimer(TIMER_CONN_CHECK);
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
    m_pMyMenuPopup = nullptr;  // 팝업은 이미 DestroyWindow 됨

    int id = (int)wParam;
    switch (id)
    {
    case MYMENU_MY_INFO:                    // ★ 개인정보 확인
    {
        MyInfoDlg dlg(this);
        dlg.DoModal();
        break;
    }
    case MYMENU_EDIT_INFO:
    {
        EditInfoDlg dlg(this);
        dlg.DoModal();
        break;
    }
    case MYMENU_POINT:
    {
        PointDlg dlg(this);
        dlg.DoModal();
        break;
    }
    case MYMENU_ADMIN_CHAT:                 // ★ 관리자 채팅
    {
        ChatDlg dlg(this);
        dlg.m_strTargetName = _T("관리자 문의");
        dlg.m_strTargetID = _T("admin");
        dlg.m_strTargetType = _T("admin");
        dlg.DoModal();
        break;
    }
    case MYMENU_LOGOUT:
    {
        CString strUserID = CA2T(
            AuthManager::GetInstance().GetCurrentUserID().c_str(), CP_UTF8);
        CString msg;
        msg.Format(_T("로그인 계정: %s\n\n로그아웃 하시겠습니까?"),
            (LPCTSTR)strUserID);
        if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES) {
            KillTimer(TIMER_CONN_CHECK);
            AuthManager::GetInstance().Logout();
            CDialogEx::OnCancel();
        }
        break;
    }
    }
    return 0;
}
