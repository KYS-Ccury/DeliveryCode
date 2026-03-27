
// ================================================================
//  OrderHistoryDlg.cpp  ─  주문 현황 화면 (서버 연동 완성본)
//
//  [연동 프로토콜]
//  ▶ 주문 내역 조회
//    REQ (203): { "token":"..." }
//    RES: { "status":2000, "orders":[
//      { "order_id":"20240522-0001",
//        "store_id":101, "store_name":"황금치킨",
//        "order_datetime":"2024-05-22 12:30",
//        "total_payment":21000, "status":1,
//        "delivery_method":"배달",
//        "items":[{"menu_name":"황금치킨","quantity":1,"price":18000}]
//      }, ...
//    ]}
//
//  ▶ 실시간 주문 상태 PUSH
//    NTF (210): { "order_id":"...", "status":2, "message":"배달 중" }
//    status: 0=접수대기, 1=조리중, 2=배달중, 3=완료, 4=취소
//
//  resource.h 사용 IDC:
//    IDC_BTN_BACK          1400
//    IDC_STATIC_ORDER_STATUS 1800
//    IDC_STATIC_ORDER_NUM  1801
//    IDC_STATIC_SHOP_NAME  1802
//    IDC_STATIC_ESTIMATED_TIME 1803
//    IDC_LIST_ORDER_ITEMS  1804
//    IDC_STATIC_FINAL_TOTAL 1805
//    IDC_BTN_CHAT          1806
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OrderHistoryDlg.h"
#include "ChatDlg.h"
#include "ReviewWriteDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

#define WM_ORDER_HISTORY_RESPONSE (WM_USER + 160)
#define WM_ORDER_STATUS_PUSH      (WM_USER + 161)

// ── 간이 JSON 파싱 ────────────────────────────────────────────
static std::string OHJStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static int OHJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); }
    catch (...) { return -1; }
}

// JSON 배열에서 객체 추출
static std::vector<std::string> OHExtractObjects(const std::string& json,
                                                  const std::string& arrayKey)
{
    std::vector<std::string> result;
    std::string token = "\"" + arrayKey + "\":[";
    auto arrPos = json.find(token);
    if (arrPos == std::string::npos) return result;
    size_t i = arrPos + token.size();
    while (i < json.size()) {
        auto objStart = json.find('{', i);
        if (objStart == std::string::npos) break;
        int depth = 0; size_t objEnd = objStart;
        for (; objEnd < json.size(); ++objEnd) {
            if (json[objEnd] == '{') ++depth;
            else if (json[objEnd] == '}') { if (--depth == 0) break; }
        }
        result.push_back(json.substr(objStart, objEnd - objStart + 1));
        i = objEnd + 1;
    }
    return result;
}

// JSON → OrderInfo 파싱
static OrderInfo ParseOrderObj(const std::string& obj)
{
    OrderInfo info;
    info.orderID        = OHJStr(obj, "order_id");
    info.storeID        = OHJInt(obj, "store_id");
    info.storeName      = OHJStr(obj, "store_name");
    info.orderDateTime  = OHJStr(obj, "order_datetime");
    info.totalPayment   = OHJInt(obj, "total_payment");
    info.deliveryStatus = OHJInt(obj, "status");
    std::string dm      = OHJStr(obj, "delivery_method");
    info.isDelivery     = (dm != "포장");
    info.deliveryAddress = OHJStr(obj, "delivery_address");
    return info;
}

// =================================================================

IMPLEMENT_DYNAMIC(OrderHistoryDlg, CDialogEx)

OrderHistoryDlg::OrderHistoryDlg(CWnd* pParent)
    : CDialogEx(IDD_ORDERHISTORY_DLG, pParent)
{}
OrderHistoryDlg::~OrderHistoryDlg() {}

void OrderHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(OrderHistoryDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,             &OrderHistoryDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_CHAT,             &OrderHistoryDlg::OnBnClickedBtnChat)
    //ON_BN_CLICKED(IDC_BTN_WRITE_REVIEW,     &OrderHistoryDlg::OnBnClickedBtnWriteReview)
    ON_MESSAGE(WM_ORDER_HISTORY_RESPONSE,   &OrderHistoryDlg::OnOrderHistoryResponse)
    ON_MESSAGE(WM_ORDER_STATUS_PUSH,        &OrderHistoryDlg::OnOrderStatusPush)
END_MESSAGE_MAP()

BOOL OrderHistoryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // ── 주문 아이템 리스트박스 초기화 ────────────────────────
    CWnd* pList = this->GetDlgItem(IDC_LIST_ORDER_ITEMS);
    if (pList) {
        // LISTBOX 이므로 CListBox 캐스트
        CListBox* pLB = static_cast<CListBox*>(pList);
        pLB->ResetContent();
    }

    // 리뷰 버튼 초기 상태: 비활성화 (배달완료 후 활성화)
    //CWnd* pReviewBtn = GetDlgItem(IDC_BTN_WRITE_REVIEW);
    //if (pReviewBtn) {
    //    pReviewBtn->EnableWindow(FALSE);
    //    pReviewBtn->ShowWindow(SW_SHOW);
    //}

    // ── 외부에서 주입된 데이터가 있으면 먼저 표시 ───────────
    if (!m_strOrderNum.IsEmpty()) {
        SetDlgItemText(IDC_STATIC_ORDER_NUM,
            _T("주문번호 : ") + m_strOrderNum);
        SetDlgItemText(IDC_STATIC_SHOP_NAME,
            _T("매장명 : ") + m_strShopName);
        CString strAmt;
        strAmt.Format(_T("총 결제금액 : %d원"), m_nTotalAmount);
        SetDlgItemText(IDC_STATIC_FINAL_TOTAL, strAmt);
        UpdateStatusBar(STATUS_WAITING);
    }

    // ── 서버 콜백 등록 ────────────────────────────────────────
    HWND hThis = GetSafeHwnd();

    // 주문 내역 조회 응답
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_ORDER_HISTORY,
        [hThis](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            ::PostMessage(hThis, WM_ORDER_HISTORY_RESPONSE, 0, (LPARAM)pBody);
        });

    // 실시간 주문 상태 PUSH
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::NTF_ORDER_STATUS,
        [hThis](uint16_t, const std::string& body) {
            int* pStatus = new int(OHJInt(body, "status"));
            ::PostMessage(hThis, WM_ORDER_STATUS_PUSH, 0, (LPARAM)pStatus);
        });

    // ── 서버에 주문 내역 요청 ─────────────────────────────────
    RequestOrderHistory();

    return TRUE;
}

// ── 서버에 주문 내역 요청 ─────────────────────────────────────
void OrderHistoryDlg::RequestOrderHistory()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json  = "{\"token\":\"" + token + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_ORDER_HISTORY, json);
}

// ── 주문 내역 서버 응답 ───────────────────────────────────────
LRESULT OrderHistoryDlg::OnOrderHistoryResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = OHJInt(*pBody, "status");
    if (status == (int)Status::SUCCESS) {
        m_vecOrders.clear();
        auto orderObjs = OHExtractObjects(*pBody, "orders");
        for (const auto& obj : orderObjs)
            m_vecOrders.push_back(ParseOrderObj(obj));

        // 가장 최근 주문(첫 번째)을 화면에 표시
        if (!m_vecOrders.empty())
            PopulateOrderInfo(m_vecOrders.front());
    }
    delete pBody;
    return 0;
}

// ── 주문 정보 UI 채우기 ───────────────────────────────────────
void OrderHistoryDlg::PopulateOrderInfo(const OrderInfo& info)
{
    CString strOrderID = CA2T(info.orderID.c_str(), CP_UTF8);
    SetDlgItemText(IDC_STATIC_ORDER_NUM,
        _T("주문번호 : ") + strOrderID);

    CString strStore = CA2T(info.storeName.c_str(), CP_UTF8);
    SetDlgItemText(IDC_STATIC_SHOP_NAME,
        _T("매장명 : ") + strStore);

    CString strAmt;
    strAmt.Format(_T("총 결제금액 : %d원"), info.totalPayment);
    SetDlgItemText(IDC_STATIC_FINAL_TOTAL, strAmt);

    CString strMethod = info.isDelivery ? _T("배달") : _T("포장(픽업)");
    CString strTime   = CA2T(info.orderDateTime.c_str(), CP_UTF8);
    CString strEst;
    strEst.Format(_T("수령 방법: %s  |  주문시간: %s"),
                  (LPCTSTR)strMethod, (LPCTSTR)strTime);
    SetDlgItemText(IDC_STATIC_ESTIMATED_TIME, strEst);

    UpdateStatusBar(info.deliveryStatus);

    // 배달 완료 상태일 때만 리뷰 작성 버튼 활성화
    //CWnd* pReviewBtn = GetDlgItem(IDC_BTN_WRITE_REVIEW);
    //if (pReviewBtn) {
    //    bool canReview = (info.deliveryStatus == STATUS_COMPLETE);
    //    pReviewBtn->EnableWindow(canReview ? TRUE : FALSE);
    //    pReviewBtn->ShowWindow(SW_SHOW);
    //}

    // 주문 아이템은 별도 REQ_ORDER_DETAIL(204)로 조회하거나
    // orders 배열 내 items 필드를 파싱해 listbox에 추가
    m_strOrderNum = strOrderID;
    m_strShopName = strStore;
    m_nTotalAmount = info.totalPayment;
}

// ── 진행 상태 바 갱신 ────────────────────────────────────────
void OrderHistoryDlg::UpdateStatusBar(int status)
{
    m_nCurrentStatus = status;
    const TCHAR* steps[] = { _T("주문접수"), _T("조리중"),
                              _T("배달중"),  _T("배달완료") };
    CString bar;
    for (int i = 0; i < 4; ++i) {
        if      (i < status)  bar += CString(_T("[V] ")) + steps[i] + _T("  ");
        else if (i == status) bar += CString(_T("[>] ")) + steps[i] + _T("  ");
        else                  bar += CString(_T("[ ] ")) + steps[i] + _T("  ");
    }
    SetDlgItemText(IDC_STATIC_ORDER_STATUS, bar);
}

// ── 실시간 상태 PUSH 수신 ─────────────────────────────────────
LRESULT OrderHistoryDlg::OnOrderStatusPush(WPARAM, LPARAM lParam)
{
    int* pStatus = reinterpret_cast<int*>(lParam);
    if (!pStatus) return 0;
    int newStatus = *pStatus;
    delete pStatus;

    UpdateStatusBar(newStatus);

    if (newStatus == STATUS_COMPLETE) {
        AfxMessageBox(_T("배달이 완료되었습니다! 맛있게 드세요!"),
                      MB_ICONINFORMATION);
        NetworkManager::GetInstance()
            .UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    } else if (newStatus == STATUS_CANCELED) {
        AfxMessageBox(_T("주문이 취소되었습니다."), MB_ICONWARNING);
        NetworkManager::GetInstance()
            .UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    }
    return 0;
}

// ── 1:1 채팅 버튼 ────────────────────────────────────────────
void OrderHistoryDlg::OnBnClickedBtnChat()
{
    ChatDlg dlg(this);
    dlg.m_strTargetName = m_strShopName.IsEmpty() ? _T("가게 문의") : m_strShopName + _T(" 문의");
    dlg.m_strTargetType = _T("owner");
    dlg.m_nOrderId      = 0;
    dlg.DoModal();
}

void OrderHistoryDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_ORDER_HISTORY);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    EndDialog(IDCANCEL);
}

// ── 리뷰 작성 버튼 ───────────────────────────────────────────
//void OrderHistoryDlg::OnBnClickedBtnWriteReview()
//{
//    // 현재 표시 중인 주문이 없으면 무시
//    if (m_vecOrders.empty() && m_strOrderNum.IsEmpty()) {
//        AfxMessageBox(_T("리뷰를 작성할 주문을 선택해주세요."), MB_ICONWARNING);
//        return;
//    }
//
//    // 배달 완료 상태 재확인
//    if (!m_vecOrders.empty() && m_vecOrders.front().deliveryStatus != STATUS_COMPLETE) {
//        AfxMessageBox(_T("배달 완료된 주문만 리뷰를 작성할 수 있습니다."), MB_ICONWARNING);
//        return;
//    }
//
//    ReviewWriteDlg dlg(this);
//    if (dlg.DoModal() == IDOK) {
//        AfxMessageBox(_T("리뷰가 등록되었습니다. 감사합니다!"), MB_ICONINFORMATION);
//        // 리뷰 작성 후 버튼 비활성화 (중복 작성 방지)
//        CWnd* pBtn = GetDlgItem(IDC_BTN_WRITE_REVIEW);
//        if (pBtn) pBtn->EnableWindow(FALSE);
//    }
//}
