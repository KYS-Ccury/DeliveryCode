
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
#include "OrderInfo.h"
#include "common/header/Types.h"
#include "json.hpp"

using json = nlohmann::json;

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
    info.orderID = OHJStr(obj, "order_id");  // 숫자면 아래 처리
    // order_id가 숫자인 경우 대응
    if (info.orderID.empty()) {
        int oid = OHJInt(obj, "order_id");
        if (oid > 0) info.orderID = std::to_string(oid);
    }
    info.storeID = OHJInt(obj, "store_id");
    info.storeName = OHJStr(obj, "store_name");
    info.orderDateTime = OHJStr(obj, "order_time");
    info.totalPayment = OHJInt(obj, "total_price");
    info.deliveryFee = OHJInt(obj, "delivery_fee");

    info.deliveryStatus = OHJInt(obj, "status");
    std::string dm = OHJStr(obj, "delivery_method");
    info.isDelivery = (dm != "PICKUP");   // ★ 서버가 "DELIVERY"/"PICKUP"으로 보냄
    info.deliveryAddress = OHJStr(obj, "delivery_address");

    // ★ items 파싱
    auto itemObjs = OHExtractObjects(obj, "items");
    for (const auto& iObj : itemObjs) {
        OrderItem item;
        item.menuName = OHJStr(iObj, "menu_name");
        item.quantity = OHJInt(iObj, "quantity");
        item.price = OHJInt(iObj, "price");

        // 옵션 파싱
        auto optObjs = OHExtractObjects(iObj, "options");
        for (const auto& oObj : optObjs) {
            OrderOptionInfo opt;
            opt.optionName = OHJStr(oObj, "option_name");
            opt.extraPrice = OHJInt(oObj, "extra_price");
            item.options.push_back(opt);
        }
        info.items.push_back(item);
    }
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
    ON_WM_TIMER()  // ★ 추가
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

    // ★ 5초마다 상태 폴링 (주문 완료/취소되면 타이머 중단)
    SetTimer(1, 5000, nullptr);

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
    // 1. lParam을 안전하게 string 포인터로 받기
    std::string* pRawBody = reinterpret_cast<std::string*>(lParam);
    if (!pRawBody || pRawBody->empty()) return 0;

    try {
        // 2. 문자열을 실제 JSON 객체로 파싱
        nlohmann::json j = nlohmann::json::parse(*pRawBody);

        // 3. 서버 응답 성공 여부 확인 (Status::SUCCESS == 2000)
        if (j.contains("status") && j["status"] == 2000) {
            m_vecOrders.clear();

            if (j.contains("orders") && j["orders"].is_array()) {
                for (auto& obj : j["orders"]) {
                    // obj(json 객체)를 OrderInfo 구조체로 변환하여 벡터에 저장
                    // ParseOrderObj가 json 객체를 직접 받도록 수정되어 있다면 obj 전달,
                    // 아니면 obj.dump() 전달
                    m_vecOrders.push_back(ParseOrderObj(obj.dump()));
                }
            }

            // 4. 데이터가 있다면 UI 갱신
            if (!m_vecOrders.empty()) {
                const OrderInfo& latestOrder = m_vecOrders.front();
                int newStatus = latestOrder.deliveryStatus;

                // ★ 상태가 바뀌었거나, 처음 데이터를 받은 경우 UI 갱신
                if (m_nCurrentStatus == -1 || newStatus != m_nCurrentStatus) {
                    m_nCurrentStatus = newStatus; // 현재 상태값 저장 (중요!)

                    // 작성하신 UI 채우기 함수 호출
                    PopulateOrderInfo(latestOrder);

                    // 상태별 알림창
                    if (newStatus == STATUS_COMPLETE) {
                        AfxMessageBox(_T("배달이 완료되었습니다!\n맛있게 드세요!"), MB_ICONINFORMATION);
                        KillTimer(1); // 완료 시 폴링 중단
                    }
                    else if (newStatus == STATUS_CANCELED) {
                        AfxMessageBox(_T("주문이 취소되었습니다."), MB_ICONWARNING);
                        KillTimer(1); // 취소 시 폴링 중단
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        // 파싱 에러 발생 시 로그 (한글 깨짐이나 키 이름 불일치 등)
        TRACE(_T("JSON Parse Error in Response: %S\n"), e.what());
    }

    // 5. 서버에서 new한 메모리 해제 (메모리 누수 방지)
    delete pRawBody;
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
    CString strTime = CA2T(info.orderDateTime.c_str(), CP_UTF8);
    CString strEst;
    strEst.Format(_T("수령방법: %s  |  주문시간: %s"),
        (LPCTSTR)strMethod, (LPCTSTR)strTime);
    SetDlgItemText(IDC_STATIC_ESTIMATED_TIME, strEst);

    UpdateStatusBar(info.deliveryStatus);

    // ★ 주문 메뉴 + 옵션 리스트박스 채우기
    CWnd* pListWnd = this->GetDlgItem(IDC_LIST_ORDER_ITEMS);
    if (pListWnd) {
        CListBox* pLB = static_cast<CListBox*>(pListWnd);
        pLB->ResetContent();

        for (const auto& item : info.items) {
            // 메뉴명 + 수량 + 가격
            CString strMenu = CA2T(item.menuName.c_str(), CP_UTF8);
            CString strLine;
            strLine.Format(_T("%s  x%d  %d원"),
                (LPCTSTR)strMenu, item.quantity, item.price * item.quantity);
            pLB->AddString(strLine);

            // 옵션 들여쓰기 표시
            for (const auto& opt : item.options) {
                CString strOpt = CA2T(opt.optionName.c_str(), CP_UTF8);
                CString strOptLine;
                strOptLine.Format(_T("    ∟ %s  +%d원"),
                    (LPCTSTR)strOpt, opt.extraPrice);
                pLB->AddString(strOptLine);
            }
        }

        // ★ 추가: 배달비 출력
        //if (info.deliveryFee > 0) {
        if (info.isDelivery && info.deliveryFee > 0) {
            pLB->AddString(_T("------------------------------------------"));
            CString strFee;
            strFee.Format(_T("배달비  +%d원"), info.deliveryFee);
            pLB->AddString(strFee);
        }

        if (info.items.empty())
            pLB->AddString(_T("주문 내역을 불러오는 중..."));
    }

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
    KillTimer(1);  // ★ 추가
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_ORDER_HISTORY);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    EndDialog(IDCANCEL);
}

void OrderHistoryDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        // 이미 완료/취소된 상태면 폴링 중단
        if (m_nCurrentStatus == STATUS_COMPLETE ||
            m_nCurrentStatus == STATUS_CANCELED) {
            KillTimer(1);
            return;
        }
        RequestOrderHistory();  // 재조회 요청
    }
    CDialogEx::OnTimer(nIDEvent);
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
