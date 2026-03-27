// ================================================================
//  OrderListDlg.cpp  ─  주문 내역 목록 화면
//
//  [화면 구성]
//  ┌──────────────────────────────────────────┐
//  │ ← 뒤로      📋 주문 내역                  │
//  ├──────────────────────────────────────────┤
//  │  [주문목록 리스트뷰 - 날짜/매장/금액/상태]  │
//  │  ...                                      │
//  ├──────────────────────────────────────────┤
//  │  선택 주문 상세 정보                        │
//  │  주문번호 / 매장명 / 일시 / 수령방법 / 상태  │
//  │  [주문 메뉴 리스트박스]                     │
//  │  총 결제금액:  OO원                         │
//  │       [리뷰 작성하기]                       │
//  └──────────────────────────────────────────┘
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OrderListDlg.h"
#include "ReviewWriteDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"
#include "json.hpp"

using json = nlohmann::json;

#define WM_ORDERLIST_RESPONSE (WM_USER + 170)

// ─── 상태 상수 ────────────────────────────────────────────────
//static constexpr int STATUS_WAITING = 0;
//static constexpr int STATUS_COOKING = 1;
//static constexpr int STATUS_DELIVERY = 2;
//static constexpr int STATUS_COMPLETE = 3;
//static constexpr int STATUS_CANCELED = 4;

// ─── 간이 JSON 파싱 ──────────────────────────────────────────
//static std::string OLJStr(const std::string& json, const std::string& key)
//{
//    std::string token = "\"" + key + "\":\"";
//    auto pos = json.find(token);
//    if (pos == std::string::npos) return "";
//    pos += token.size();
//    auto end = json.find('"', pos);
//    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
//}
//
//static int OLJInt(const std::string& json, const std::string& key)
//{
//    std::string token = "\"" + key + "\":";
//    auto pos = json.find(token);
//    if (pos == std::string::npos) return -1;
//    try { return std::stoi(json.substr(pos + token.size())); }
//    catch (...) { return -1; }
//}
//
//// JSON 배열에서 객체 추출
//static std::vector<std::string> OLExtractObjects(const std::string& json,
//    const std::string& arrayKey)
//{
//    std::vector<std::string> result;
//    std::string token = "\"" + arrayKey + "\":[";
//    auto arrPos = json.find(token);
//    if (arrPos == std::string::npos) return result;
//    size_t i = arrPos + token.size();
//    while (i < json.size()) {
//        auto objStart = json.find('{', i);
//        if (objStart == std::string::npos) break;
//        int depth = 0; size_t objEnd = objStart;
//        for (; objEnd < json.size(); ++objEnd) {
//            if (json[objEnd] == '{')      ++depth;
//            else if (json[objEnd] == '}') { if (--depth == 0) break; }
//        }
//        result.push_back(json.substr(objStart, objEnd - objStart + 1));
//        i = objEnd + 1;
//    }
//    return result;
//}
//
//// JSON → OrderInfo 파싱
//// OrderListDlg.cpp 내의 함수 수정
//static OrderInfo ParseOLOrderObj(const std::string& obj)
//{
//    OrderInfo info;
//    info.orderID = OLJStr(obj, "order_id");
//    info.storeID = OLJInt(obj, "store_id");
//    info.storeName = OLJStr(obj, "store_name");
//
//    // ★ 서버 코드와 Key 이름 매칭 (order_datetime -> order_time)
//    info.orderDateTime = OLJStr(obj, "order_time");
//
//    // ★ 서버 코드와 Key 이름 매칭 (total_payment -> total_price)
//    info.totalPayment = OLJInt(obj, "total_price");
//
//    info.deliveryStatus = OLJInt(obj, "status");
//
//    std::string dm = OLJStr(obj, "delivery_method");
//    info.isDelivery = (dm != "PICKUP"); // 보통 서버는 영문 대문자로 보냅니다.
//    info.deliveryAddress = OLJStr(obj, "delivery_address");
//
//    // 메뉴 아이템 파싱 (서버는 "items"라는 키로 보냄)
//    auto itemObjs = OLExtractObjects(obj, "items");
//    for (const auto& item : itemObjs) {
//        OrderItem oi;
//        oi.menuName = OLJStr(item, "menu_name");
//        oi.quantity = OLJInt(item, "quantity");
//
//        // ★ 서버에서 메뉴 가격 키는 "price"입니다.
//        oi.price = OLJInt(item, "price");
//
//        info.items.push_back(oi);
//    }
//    return info;
//}

// JSON -> OrderInfo 파싱 (nlohmann/json 버전)
static OrderInfo ParseOLOrderObj(const json& j)
{
    OrderInfo info;

    try {
        // 1. 주문 기본 정보
        if (j.contains("order_id")) {
            if (j["order_id"].is_number())
                info.orderID = std::to_string(j["order_id"].get<int>());
            else
                info.orderID = j.value("order_id", "");
        }

        info.storeName = j.value("store_name", "");
        info.orderDateTime = j.value("order_time", "");
        info.totalPayment = j.value("total_price", 0);
        info.deliveryStatus = (DeliveryStatus)j.value("status", 0);

        // 2. 메뉴 및 옵션 파싱 (이 부분이 핵심입니다)
        if (j.contains("items") && j["items"].is_array()) {
            for (const auto& item : j["items"]) {
                OrderItem oi;
                oi.menuName = item.value("menu_name", "");
                oi.quantity = item.value("quantity", 0);
                oi.price = item.value("price", 0);

                // ★ 추가: 옵션 배열 파싱 (서버의 "options" 키와 매칭)
                if (item.contains("options") && item["options"].is_array()) {
                    for (const auto& optJson : item["options"]) {
                        OrderOptionInfo optInfo;
                        optInfo.optionName = optJson.value("option_name", "");
                        optInfo.extraPrice = optJson.value("extra_price", 0);
                        oi.options.push_back(optInfo); // OrderItem의 options 벡터에 저장
                    }
                }

                info.items.push_back(oi);
            }
        }
    }
    catch (const std::exception& e) {
        OutputDebugStringA(("JSON Parse Error: " + std::string(e.what())).c_str());
    }

    return info;
}

// =================================================================

IMPLEMENT_DYNAMIC(OrderListDlg, CDialogEx)

OrderListDlg::OrderListDlg(CWnd* pParent)
    : CDialogEx(IDD_ORDERLIST_DLG, pParent)
{
}

OrderListDlg::~OrderListDlg() 
{
    m_brushBg.CreateSolidBrush(RGB(245, 247, 250));
}

void OrderListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_ORDER_HISTORY, m_listHistory);
    DDX_Control(pDX, IDC_LIST_OL_ITEMS, m_listItems);
}

BEGIN_MESSAGE_MAP(OrderListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK, &OrderListDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_OL_WRITE_REVIEW, &OrderListDlg::OnBnClickedBtnWriteReview)
    ON_NOTIFY(NM_CLICK, IDC_LIST_ORDER_HISTORY, &OrderListDlg::OnNMClickListOrderHistory)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_ORDER_HISTORY, &OrderListDlg::OnCustomDrawList)
    ON_MESSAGE(WM_ORDERLIST_RESPONSE, &OrderListDlg::OnOrderHistoryResponse)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// ── 초기화 ───────────────────────────────────────────────────
BOOL OrderListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 1. 제목용 큰 폰트 설정
    m_fontTitle.CreatePointFont(120, _T("맑은 고딕"));
    GetDlgItem(IDC_STATIC_OL_TOTAL)->SetFont(&m_fontTitle);

    // 2. 버튼 및 주요 텍스트 강조 폰트
    m_fontBold.CreatePointFont(100, _T("맑은 고딕"));
    GetDlgItem(IDC_BTN_OL_WRITE_REVIEW)->SetFont(&m_fontBold);

    // 3. 리스트뷰 헤더 세팅
    m_listHistory.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER); // 더블버퍼로 깜빡임 방지

    // ── 주문 목록 리스트뷰 컬럼 설정 ─────────────────────────
    m_listHistory.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listHistory.InsertColumn(0, _T("주문일시"), LVCFMT_LEFT, 130);
    m_listHistory.InsertColumn(1, _T("매장명"), LVCFMT_LEFT, 95);
    m_listHistory.InsertColumn(2, _T("결제금액"), LVCFMT_RIGHT, 80);
    m_listHistory.InsertColumn(3, _T("상태"), LVCFMT_CENTER, 68);

    // ── 상세 패널 초기화 ──────────────────────────────────────
    ClearDetailPanel();

    // ── 서버 콜백 등록 → 응답은 메인 스레드에서 처리 ─────────
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(
        CmdCustomer::REQ_ORDER_HISTORY,
        [hThis](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            ::PostMessage(hThis, WM_ORDERLIST_RESPONSE, 0, (LPARAM)pBody);
        });

    // ── 서버 요청 ─────────────────────────────────────────────
    RequestOrderHistory();

    return TRUE;
}

// ── 서버에 주문 내역 요청 ─────────────────────────────────────
void OrderListDlg::RequestOrderHistory()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인: 빈 목록 표시
        m_listHistory.DeleteAllItems();
        m_listHistory.InsertItem(0, _T("서버에 연결되어 있지 않습니다."));
        return;
    }

    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json = "{\"token\":\"" + token + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER,
        CmdCustomer::REQ_ORDER_HISTORY, json);

    // 로딩 표시
    m_listHistory.DeleteAllItems();
    m_listHistory.InsertItem(0, _T("불러오는 중..."));
}

// ── 서버 응답 처리 ───────────────────────────────────────────
LRESULT OrderListDlg::OnOrderHistoryResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    try {
        // 전체 응답 본문을 JSON으로 파싱
        auto jRes = json::parse(*pBody);

        // 서버의 Status::SUCCESS (2000) 확인
        int status = jRes.value("status", -1);

        if (status == 2000) { // Status::SUCCESS
            m_vecOrders.clear();

            // "orders" 배열을 순회하며 파싱
            if (jRes.contains("orders") && jRes["orders"].is_array()) {
                for (const auto& oJson : jRes["orders"]) {
                    m_vecOrders.push_back(ParseOLOrderObj(oJson));
                }
            }
            RebuildOrderListUI();
        }
        else {
            m_listHistory.DeleteAllItems();
            m_listHistory.InsertItem(0, _T("주문 내역이 없거나 가져오지 못했습니다."));
        }
    }
    catch (json::parse_error& e) {
        // JSON 형식이 잘못되었을 때 예외 처리
        OutputDebugStringA(e.what());
        m_listHistory.DeleteAllItems();
        m_listHistory.InsertItem(0, _T("데이터 파싱 오류가 발생했습니다."));
    }

    delete pBody;
    return 0;
}

// ── 목록 UI 재구성 ────────────────────────────────────────────
void OrderListDlg::RebuildOrderListUI()
{
    m_listHistory.DeleteAllItems();

    if (m_vecOrders.empty()) {
        // 빈 목록 안내 텍스트 표시
        CWnd* pEmpty = GetDlgItem(IDC_STATIC_OL_EMPTY);
        if (pEmpty) pEmpty->ShowWindow(SW_SHOW);
        return;
    }

    // 빈 목록 안내 숨기기
    CWnd* pEmpty = GetDlgItem(IDC_STATIC_OL_EMPTY);
    if (pEmpty) pEmpty->ShowWindow(SW_HIDE);

    for (int i = 0; i < (int)m_vecOrders.size(); ++i) {
        const auto& o = m_vecOrders[i];

        CString strDate = CA2T(o.orderDateTime.c_str(), CP_UTF8);
        CString strStore = CA2T(o.storeName.c_str(), CP_UTF8);
        CString strAmt;  strAmt.Format(_T("%d원"), o.totalPayment);
        CString strStat = GetStatusText(o.deliveryStatus);

        int r = m_listHistory.InsertItem(i, strDate);
        m_listHistory.SetItemText(r, 1, strStore);
        m_listHistory.SetItemText(r, 2, strAmt);
        m_listHistory.SetItemText(r, 3, strStat);
        m_listHistory.SetItemData(r, (DWORD_PTR)i);  // 인덱스 보존
    }

    // 첫 번째 항목 자동 선택
    if (!m_vecOrders.empty()) {
        m_listHistory.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
        PopulateDetailPanel(0);
    }
}

// ── 목록 항목 클릭 ───────────────────────────────────────────
void OrderListDlg::OnNMClickListOrderHistory(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int item = pNMIA->iItem;
    if (item >= 0 && item < (int)m_vecOrders.size()) {
        int idx = (int)m_listHistory.GetItemData(item);
        PopulateDetailPanel(idx);
    }
    *pResult = 0;
}

// ── 상세 패널 채우기 ─────────────────────────────────────────
void OrderListDlg::PopulateDetailPanel(int index)
{
    if (index < 0 || index >= (int)m_vecOrders.size()) return;
    m_nSelectedIdx = index;
    const OrderInfo& o = m_vecOrders[index];

    // 주문번호
    CString strID = CA2T(o.orderID.c_str(), CP_UTF8);
    SetDlgItemText(IDC_STATIC_OL_ORDER_NUM,
        _T("주문번호 : ") + strID);

    // 매장명
    CString strStore = CA2T(o.storeName.c_str(), CP_UTF8);
    SetDlgItemText(IDC_STATIC_OL_STORE_NAME,
        _T("매장명 : ") + strStore);

    // 주문 일시
    CString strDate = CA2T(o.orderDateTime.c_str(), CP_UTF8);
    SetDlgItemText(IDC_STATIC_OL_DATETIME,
        _T("주문일시 : ") + strDate);

    // 수령 방법
    CString strMethod = o.isDelivery ? _T("배달") : _T("포장(픽업)");
    SetDlgItemText(IDC_STATIC_OL_METHOD,
        _T("수령방법 : ") + strMethod);

    // 상태
    SetDlgItemText(IDC_STATIC_OL_STATUS, GetStatusText(o.deliveryStatus));

    // 주문 메뉴 목록
    m_listItems.ResetContent();
    for (const auto& item : o.items) {
        CString strLine;
        CString strName = CA2T(item.menuName.c_str(), CP_UTF8);
        strLine.Format(_T("%s  x%d  %d원"),
            (LPCTSTR)strName, item.quantity, item.price * item.quantity);
        m_listItems.AddString(strLine);

        for (const auto& opt : item.options) {
            CString strOpt;
            CString strOptName = CA2T(opt.optionName.c_str(), CP_UTF8);
            // └ 기호를 넣어 메뉴 아래에 포함된 옵션임을 표시
            strOpt.Format(_T("   └ %s (+%d원)"),
                (LPCTSTR)strOptName, opt.extraPrice);
            m_listItems.AddString(strOpt);
        }
    }

    // 총 결제금액
    CString strTotal;
    strTotal.Format(_T("총 결제금액 : %d원"), o.totalPayment);
    SetDlgItemText(IDC_STATIC_OL_TOTAL, strTotal);

    // 리뷰 작성 버튼 활성화 여부 (배달 완료 상태만)
    CWnd* pBtn = GetDlgItem(IDC_BTN_OL_WRITE_REVIEW);
    if (pBtn) {
        bool canReview = (o.deliveryStatus == STATUS_COMPLETE);
        pBtn->EnableWindow(canReview ? TRUE : FALSE);
    }
}

// ── 상세 패널 초기화 ─────────────────────────────────────────
void OrderListDlg::ClearDetailPanel()
{
    SetDlgItemText(IDC_STATIC_OL_ORDER_NUM, _T("주문번호 : -"));
    SetDlgItemText(IDC_STATIC_OL_STORE_NAME, _T("매장명 : -"));
    SetDlgItemText(IDC_STATIC_OL_DATETIME, _T("주문일시 : -"));
    SetDlgItemText(IDC_STATIC_OL_METHOD, _T("수령방법 : -"));
    SetDlgItemText(IDC_STATIC_OL_STATUS, _T("-"));
    SetDlgItemText(IDC_STATIC_OL_TOTAL, _T("총 결제금액 : -"));
    m_listItems.ResetContent();

    CWnd* pBtn = GetDlgItem(IDC_BTN_OL_WRITE_REVIEW);
    if (pBtn) pBtn->EnableWindow(FALSE);
}

// ── 상태 텍스트 / 색상 변환 ─────────────────────────────────
CString OrderListDlg::GetStatusText(int status) const
{
    switch (status) {
    case STATUS_WAITING:  return _T("접수 대기");
    case STATUS_PREPARING:  return _T("조리 중");
    case STATUS_DELIVERING: return _T("배달 중");
    case STATUS_COMPLETE: return _T("배달 완료");
    case STATUS_CANCELED: return _T("주문 취소");
    default:              return _T("알 수 없음");
    }
}

COLORREF OrderListDlg::GetStatusColor(int status) const
{
    switch (status) {
    case STATUS_WAITING:  return RGB(180, 120, 0);  // 주황
    case STATUS_PREPARING:  return RGB(0, 120, 200);  // 파랑
    case STATUS_DELIVERING: return RGB(20, 150, 80);  // 초록
    case STATUS_COMPLETE: return RGB(100, 100, 100);  // 회색
    case STATUS_CANCELED: return RGB(200, 0, 0);  // 빨강
    default:              return RGB(0, 0, 0);
    }
}

// ── 리스트뷰 커스텀 드로우: 상태 열 색상 ─────────────────────
void OrderListDlg::OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT:
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
        // 3번 열(상태)만 색상 적용
        if (pCD->iSubItem == 3) {
            int idx = (int)m_listHistory.GetItemData((int)pCD->nmcd.dwItemSpec);
            if (idx >= 0 && idx < (int)m_vecOrders.size()) {
                pCD->clrText = GetStatusColor(m_vecOrders[idx].deliveryStatus);
            }
        }
        *pResult = CDRF_DODEFAULT;
        break;
    }
    }
}

// ── 배경색 ───────────────────────────────────────────────────
HBRUSH OrderListDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    // 상태 스태틱: 상태 색상 적용
    if (nCtlColor == CTLCOLOR_STATIC &&
        pWnd && pWnd->GetDlgCtrlID() == IDC_STATIC_OL_STATUS)
    {
        if (m_nSelectedIdx >= 0 && m_nSelectedIdx < (int)m_vecOrders.size()) {
            pDC->SetTextColor(
                GetStatusColor(m_vecOrders[m_nSelectedIdx].deliveryStatus));
        }
        pDC->SetBkMode(TRANSPARENT);
        return m_brushBg;
    }
    if (nCtlColor == CTLCOLOR_DLG)    return m_brushBg;
    if (nCtlColor == CTLCOLOR_STATIC) {
        pDC->SetBkMode(TRANSPARENT);
        return m_brushBg;
    }
    return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

// ── 리뷰 작성 버튼 ───────────────────────────────────────────
void OrderListDlg::OnBnClickedBtnWriteReview()
{
    if (m_nSelectedIdx < 0 || m_nSelectedIdx >= (int)m_vecOrders.size()) {
        AfxMessageBox(_T("리뷰를 작성할 주문을 선택해주세요."), MB_ICONWARNING);
        return;
    }
    const OrderInfo& o = m_vecOrders[m_nSelectedIdx];
    if (o.deliveryStatus != STATUS_COMPLETE) {
        AfxMessageBox(_T("배달 완료된 주문만 리뷰를 작성할 수 있습니다."),
            MB_ICONWARNING);
        return;
    }

    ReviewWriteDlg dlg(this);
    // ReviewWriteDlg에 주문 정보 전달 (해당 클래스의 공개 멤버에 맞게 조정)
    // dlg.m_strOrderID  = CA2T(o.orderID.c_str(), CP_UTF8);
    // dlg.m_nStoreID    = o.storeID;
    if (dlg.DoModal() == IDOK) {
        AfxMessageBox(_T("리뷰가 등록되었습니다. 감사합니다!"), MB_ICONINFORMATION);
        // 리뷰 작성 후 버튼 비활성화 (중복 방지)
        CWnd* pBtn = GetDlgItem(IDC_BTN_OL_WRITE_REVIEW);
        if (pBtn) pBtn->EnableWindow(FALSE);
    }
}

// ── 뒤로가기 ─────────────────────────────────────────────────
void OrderListDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance()
        .UnregisterCallback(CmdCustomer::REQ_ORDER_HISTORY);
    EndDialog(IDCANCEL);
}