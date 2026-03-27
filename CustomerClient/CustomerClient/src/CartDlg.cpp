// ================================================================
//  CartDlg.cpp  ─  장바구니 / 주문하기 (완성판)
//
//  [변경/완성 사항]
//  1. 배달 주소를 AuthManager에서 가져와 자동 입력
//  2. 포인트 사용 입력 에디트 연동 (IDC_EDIT_USE_POINT)
//  3. 쿠폰 입력 에디트 연동 (IDC_EDIT_COUPON_ID)
//  4. BuildOrderJson에 배달 주소/포인트/쿠폰 실제 값 전달
//  5. 옵션 수정(OptionChangeDlg) 완료 후 옵션 목록 동기화
//  6. 서버 응답에서 estimated_minutes 가 없는 경우 기본값 30 처리
//
//  [IDC 목록] ← resource.h 에 추가 필요
//    IDC_LIST_CART              기존
//    IDC_BTN_BACK               기존
//    IDC_BTN_EDIT_OPTION        기존
//    IDC_RADIO_DELIVERY         기존
//    IDC_RADIO_PICKUP           기존
//    IDC_STATIC_ORDER_PRICE     기존
//    IDC_STATIC_DELIVERY_FEE    기존
//    IDC_STATIC_TOTAL_PRICE     기존
//    IDC_EDIT_DELIVERY_REQUEST  기존
//    IDC_STATIC_REQUEST_LABEL   기존
//    IDC_EDIT_DELIVERY_ADDR     2020  ← 배달 주소 입력
//    IDC_STATIC_ADDR_LABEL      2021  ← 배달 주소 라벨
//    IDC_EDIT_USE_POINT         2022  ← 포인트 사용 입력
//    IDC_STATIC_MY_POINT        2023  ← 보유 포인트 표시
//    IDC_EDIT_COUPON_ID         2024  ← 쿠폰 ID 입력
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "CartDlg.h"
#include "AddressDlg.h"
#include "AddressManager.h"
#include "DeliveryOkDlg.h"
#include "OptionChangeDlg.h"
#include "OrderManager.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "PaymentDlg.h"
#include "common/header/Types.h"
#include "json.hpp"

using json = nlohmann::json;

// ── IDC 임시 정의 ─────────────────────────────────────────────
#ifndef IDC_EDIT_DELIVERY_ADDR
#define IDC_EDIT_DELIVERY_ADDR  2020
#define IDC_STATIC_ADDR_LABEL   2021
#define IDC_EDIT_USE_POINT      2022
#define IDC_STATIC_MY_POINT     2023
#define IDC_EDIT_COUPON_ID      2024
#endif

#define WM_ORDER_RESPONSE (WM_USER + 130)

// ── JSON 이스케이프 ───────────────────────────────────────────
static std::string EscJ(const std::string& s)
{
    std::string o;
    for (char c : s) { if (c == '"') o += "\\\""; else o += c; }
    return o;
}

// ── 주문 JSON 빌드 ────────────────────────────────────────────
static std::string BuildOrderJson(
    const std::vector<CartItem>& cart,
    int storeID,
    bool isDelivery,
    const std::string& deliveryAddr,
    const std::string& request,
    int payMethodID,
    int usePoint,
    int couponID)
{
    json j;
    j["store_id"] = storeID;
    j["is_delivery"] = isDelivery; // 서버가 찾던 바로 그 키!
    j["delivery_method"] = isDelivery ? "DELIVERY" : "PICKUP";

    if (isDelivery) {
        j["delivery_address"] = deliveryAddr;
        j["delivery_request"] = request;
    }

    j["payment_method_id"] = payMethodID;
    j["use_point"] = usePoint;
    j["coupon_id"] = couponID;

    j["items"] = json::array();
    for (const auto& item : cart) {
        json menu;
        menu["menu_id"] = item.menuID;
        menu["name"] = item.menuName;
        menu["price"] = item.basePrice;
        menu["qty"] = item.quantity;

        menu["options"] = json::array();
        for (const auto& opt : item.selectedOptions) {
            json o;
            o["option_item_id"] = opt.optionID;
            o["option_name"] = opt.optionName;
            o["extra_price"] = opt.optionPrice;
            menu["options"].push_back(o);
        }
        j["items"].push_back(menu);
    }

    return j.dump(); // 자동으로 완벽한 JSON 문자열 생성
}

// ── 응답 파싱 헬퍼 ────────────────────────────────────────────
//static std::string ParseOID(const std::string& b)
//{
//    // order_id 가 숫자인 경우: "order_id":123
//    std::string t1 = "\"order_id\":\"";
//    auto p1 = b.find(t1);
//    if (p1 != std::string::npos) {
//        p1 += t1.size();
//        auto e = b.find('"', p1);
//        return (e == std::string::npos) ? "" : b.substr(p1, e - p1);
//    }
//    std::string t2 = "\"order_id\":";
//    auto p2 = b.find(t2);
//    if (p2 != std::string::npos) {
//        p2 += t2.size();
//        auto e = b.find_first_of(",}", p2);
//        return (e == std::string::npos) ? "" : b.substr(p2, e - p2);
//    }
//    return "";
//}
//static int ParseStat(const std::string& b)
//{
//    std::string t = "\"status\":";
//    auto p = b.find(t);
//    if (p == std::string::npos) return -1;
//    try { return std::stoi(b.substr(p + t.size())); } catch (...) { return -1; }
//}
//static int ParseEst(const std::string& b)
//{
//    std::string t = "\"estimated_minutes\":";
//    auto p = b.find(t);
//    if (p == std::string::npos) return 30;
//    try { return std::stoi(b.substr(p + t.size())); } catch (...) { return 30; }
//}

// =================================================================

IMPLEMENT_DYNAMIC(CartDlg, CDialogEx)
CartDlg::CartDlg(CWnd* pParent) : CDialogEx(IDD_CART_DLG, pParent) {}
CartDlg::~CartDlg() {}

void CartDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_CART, m_listCart);
}

BEGIN_MESSAGE_MAP(CartDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                &CartDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BTN_ADDR_CHANGE, &CartDlg::OnBnClickedBtnAddrChange)
    ON_BN_CLICKED(IDC_BTN_BACK,        &CartDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_EDIT_OPTION, &CartDlg::OnBnClickedBtnEditOption)
    ON_BN_CLICKED(IDC_RADIO_DELIVERY,  &CartDlg::OnBnClickedRadioDelivery)
    ON_BN_CLICKED(IDC_RADIO_PICKUP,    &CartDlg::OnBnClickedRadioPickup)
    ON_MESSAGE(WM_ORDER_RESPONSE,      &CartDlg::OnOrderResponse)
    ON_EN_CHANGE(IDC_EDIT_USE_POINT, &CartDlg::OnEnChangeEditUsePoint)
END_MESSAGE_MAP()

BOOL CartDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_CAPTION, 0);
    CenterWindow();

    // 장바구니 리스트 컬럼
    m_listCart.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listCart.InsertColumn(0, _T("메뉴명"),  LVCFMT_LEFT,  150);
    m_listCart.InsertColumn(1, _T("가격"),    LVCFMT_RIGHT,  90);
    m_listCart.InsertColumn(2, _T("수량"),    LVCFMT_CENTER, 40);
    m_listCart.InsertColumn(3, _T("옵션"),    LVCFMT_LEFT,  110);

    // 배달/포장 라디오 초기값
    m_bDelivery = true;
    CheckDlgButton(IDC_RADIO_DELIVERY, BST_CHECKED);
    CheckDlgButton(IDC_RADIO_PICKUP,   BST_UNCHECKED);

    // ── 배달 주소: AuthManager에서 자동 입력 ────────────────
    // AuthManager에 GetCurrentAddress() 가 없으면
    // REQ_GET_PROFILE 응답에서 받은 address 사용
    // (일단 빈 칸으로 두고, 사용자가 직접 입력)
    // ── 배달 주소: AddressManager 기본 주소 자동 입력 ──────
    CWnd* pAddr = this->GetDlgItem(IDC_EDIT_DELIVERY_ADDR);
    if (pAddr) {
        std::string defAddr = AddressManager::GetInstance().GetDefaultAddress();
        if (!defAddr.empty()) {
            pAddr->SetWindowText(CA2T(defAddr.c_str(), CP_UTF8));
        } else {
            pAddr->SetWindowText(_T(""));
            ::SendMessage(pAddr->GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                          (LPARAM)_T("배달 받을 주소를 입력하세요"));
        }
    }
    // ── 주소 옆에 "변경" 버튼 역할: 주소 클릭 시 AddressDlg 열기 ─
    // IDC_STATIC_ADDR_LABEL 클릭 시 주소 선택 다이얼로그

    // ── 요청사항 힌트 텍스트 ─────────────────────────────────
    CWnd* pReq = this->GetDlgItem(IDC_EDIT_DELIVERY_REQUEST);
    if (pReq) {
        ::SendMessage(pReq->GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                      (LPARAM)_T("예: 문 앞에 놓아주세요 / 벨 누르지 마세요"));
    }

    // ── 포인트: 보유 포인트 표시 ─────────────────────────────
    // 서버 연결 시 로그인 응답에서 받은 포인트 표시
    // (OrderManager::GetMyPoints()는 현재 더미 반환 - 서버 연동 후 실값)
    int myPoint = OrderManager::GetInstance().GetMyPoints();
    CWnd* pPtLabel = this->GetDlgItem(IDC_STATIC_MY_POINT);
    if (pPtLabel) {
        CString strPt;
        strPt.Format(_T("보유 포인트: %d원"), myPoint);
        pPtLabel->SetWindowText(strPt);
    }
    CWnd* pPtEdit = this->GetDlgItem(IDC_EDIT_USE_POINT);
    if (pPtEdit) {
        pPtEdit->SetWindowText(_T("0"));
        ::SendMessage(pPtEdit->GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                      (LPARAM)_T("0"));
    }

    // ── 쿠폰 힌트 ────────────────────────────────────────────
    CWnd* pCoupon = this->GetDlgItem(IDC_EDIT_COUPON_ID);
    if (pCoupon) {
        ::SendMessage(pCoupon->GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                      (LPARAM)_T("쿠폰 번호 (없으면 빈칸)"));
    }

    // ── 결제수단 콤보박스 채우기 ─────────────────────────────
    LoadPaymentCards();

    // ── 주문 생성 응답 콜백 등록 ─────────────────────────────
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_CREATE_ORDER,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_ORDER_RESPONSE, 0, (LPARAM)p);
        });

    // 장바구니 로드
    m_vecCart = OrderManager::GetInstance().GetCartItems();
    UpdateCartUI();
    return TRUE;
}

// ── 장바구니 UI 갱신 ─────────────────────────────────────────
void CartDlg::UpdateCartUI()
{
    m_listCart.DeleteAllItems();
    int total = 0;
    int currentStoreID = -1; // 현재 장바구니의 가게 ID 저장용

    for (int i = 0; i < (int)m_vecCart.size(); ++i) {
        m_vecCart[i].CalculateTotalPrice();
        CString n  = CA2T(m_vecCart[i].menuName.c_str(), CP_UTF8);
        CString pr; pr.Format(_T("%d원"), m_vecCart[i].totalPrice);
        CString q;  q.Format(_T("%d"),    m_vecCart[i].quantity);
        CString op;
        for (const auto& opt : m_vecCart[i].selectedOptions) {
            if (!op.IsEmpty()) op += _T(",");
            op += CA2T(opt.optionName.c_str(), CP_UTF8);
        }
        int r = m_listCart.InsertItem(i, n);
        m_listCart.SetItemText(r, 1, pr);
        m_listCart.SetItemText(r, 2, q);
        m_listCart.SetItemText(r, 3, op);
        total += m_vecCart[i].totalPrice;
        currentStoreID = m_vecCart[i].storeID; // 가게 ID 확보
    }

    // ★ 수정: 고정 3000원 대신 DB에서 로드된 배달비 사용
    int fee = 0;
    if (m_bDelivery && currentStoreID != -1) {
        // OrderManager 등에 저장된 가게 리스트에서 해당 가게의 deliveryFee를 찾아옵니다.
        // (가게 목록을 불러올 때 이미 delivery_fee를 받아왔다는 가정)
        auto& om = OrderManager::GetInstance();
        // GetStoreDeliveryFee 같은 함수가 있다면 사용, 없다면 m_allStores 등에서 검색
        fee = om.GetDeliveryFeeByStore(currentStoreID);
    }

    // 추가: 에디트 박스에서 현재 입력된 포인트 읽어오기
    CWnd* pPtEdit = GetDlgItem(IDC_EDIT_USE_POINT);
    if (pPtEdit) {
        CString strPt;
        pPtEdit->GetWindowText(strPt);
        m_nUsePoint = _ttoi(strPt);
    }
    int final_ = total + fee - m_nUsePoint; // 포인트 차감까지 반영

    CString s;
    s.Format(_T("%d원"), total);   SetDlgItemText(IDC_STATIC_ORDER_PRICE,  s);
    s.Format(_T("%d원"), fee);     SetDlgItemText(IDC_STATIC_DELIVERY_FEE, s);
    s.Format(_T("%d원"), final_);  SetDlgItemText(IDC_STATIC_TOTAL_PRICE,  s);
    s.Format(_T("주문하기 (%d원)"), final_);
    SetDlgItemText(IDOK, s);

    // 배달 관련 컨트롤 표시/숨김
    auto ShowCtrl = [&](int nID, bool bShow) {
        CWnd* p = this->GetDlgItem(nID);
        if (p) p->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
    };
    ShowCtrl(IDC_STATIC_REQUEST_LABEL,  m_bDelivery);
    ShowCtrl(IDC_EDIT_DELIVERY_REQUEST, m_bDelivery);
    ShowCtrl(IDC_STATIC_ADDR_LABEL,     m_bDelivery);
    ShowCtrl(IDC_EDIT_DELIVERY_ADDR,    m_bDelivery);
}

// ── 주문하기 버튼 ─────────────────────────────────────────────
void CartDlg::OnBnClickedOk()
{
    if (m_bWaiting.load()) return;

    if (m_vecCart.empty()) {
        AfxMessageBox(_T("장바구니가 비어있습니다."), MB_ICONWARNING);
        return;
    }

    if (m_vecCardIDs.empty()) {
        AfxMessageBox(_T("등록된 결제수단이 없습니다.\n마이페이지에서 카드를 먼저 등록해 주세요."),
            MB_ICONWARNING);
        return;
    }

    // ── 결제수단 선택 여부 확인 ★ 신규 ───────────────
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_PAYMENT);
    if (!pCombo || pCombo->GetCurSel() == CB_ERR) {
        AfxMessageBox(_T("결제수단을 선택해 주세요.\n결제수단은 홈 > 결제수단 관리에서 등록할 수 있습니다."),
            MB_ICONWARNING);
        return;
    }

    // ── 배달 주소 읽기 ────────────────────────────────────────
    CString strAddr;
    if (m_bDelivery) {
        CWnd* pAddr = this->GetDlgItem(IDC_EDIT_DELIVERY_ADDR);
        if (pAddr) pAddr->GetWindowText(strAddr);
        strAddr.Trim();
        if (strAddr.IsEmpty()) {
            AfxMessageBox(_T("배달 주소를 입력해 주세요."), MB_ICONWARNING);
            if (pAddr) pAddr->SetFocus();
            return;
        }
    }

    // ── 요청사항 읽기 ────────────────────────────────────────
    CWnd* pReq = this->GetDlgItem(IDC_EDIT_DELIVERY_REQUEST);
    if (pReq) pReq->GetWindowText(m_strDeliveryRequest);
    m_strDeliveryRequest.Trim();

    // ── 포인트 사용액 읽기 ────────────────────────────────────
    int usePoint = 0;
    CWnd* pPt = this->GetDlgItem(IDC_EDIT_USE_POINT);
    if (pPt) {
        CString strPt;
        pPt->GetWindowText(strPt);
        usePoint = _ttoi(strPt);
        if (usePoint < 0) usePoint = 0;
        // 보유 포인트 초과 방지
        int myPoint = OrderManager::GetInstance().GetMyPoints();
        if (usePoint > myPoint) {
            CString msg;
            msg.Format(_T("보유 포인트(%d원)를 초과하여 사용할 수 없습니다."), myPoint);
            AfxMessageBox(msg, MB_ICONWARNING);
            return;
        }
    }

    // ── 쿠폰 ID 읽기 ─────────────────────────────────────────
    int couponID = 0;
    CWnd* pCoupon = this->GetDlgItem(IDC_EDIT_COUPON_ID);
    if (pCoupon) {
        CString strCoupon;
        pCoupon->GetWindowText(strCoupon);
        strCoupon.Trim();
        if (!strCoupon.IsEmpty())
            couponID = _ttoi(strCoupon);
    }

    auto& net = NetworkManager::GetInstance();

    // ── 오프라인 테스트 ────────────────────────────────────────
    if (!net.IsConnected()) {
        // 장바구니에서 주문 정보 구성
        CString strOrderList;
        for (const auto& item : m_vecCart) {
            CString name = CA2T(item.menuName.c_str(), CP_UTF8);
            CString line;
            line.Format(_T("%s x%d\n"), (LPCTSTR)name, item.quantity);
            strOrderList += line;
        }
        int total = OrderManager::GetInstance().GetTotalAmount();

        OrderManager::GetInstance().ClearCart();

        // 현재 시각 구성
        CTime now = CTime::GetCurrentTime();
        CString strNow = now.Format(_T("%Y-%m-%d %H:%M"));

        // 결제 수단명 읽기
        CString strPayLabel;
        CWnd* pComboWnd = this->GetDlgItem(IDC_COMBO_PAYMENT);
        if (pComboWnd) static_cast<CComboBox*>(pComboWnd)->GetWindowText(strPayLabel);
        if (strPayLabel.IsEmpty()) strPayLabel = _T("테스트 결제");

        int delivFee = m_bDelivery ? 3000 : 0;

        DeliveryOkDlg dlg(this);
        dlg.m_strOrderID        = _T("TEST-0001");
        dlg.m_nEstimatedMinutes = 30;
        dlg.m_strStoreName      = _T("테스트 가게");
        dlg.m_strOrderList      = strOrderList;
        dlg.m_nTotalAmount      = total + delivFee - usePoint;
        dlg.m_strDeliveryAddr   = m_bDelivery ? strAddr : _T("포장(픽업)");
        dlg.m_strOrderDateTime  = strNow;
        dlg.m_nUsedPoint        = usePoint;
        dlg.m_nDeliveryFee      = delivFee;
        dlg.m_strPayMethod      = strPayLabel;

        // ★ 신규: 옵션 포함 주문 항목 구성
        for (const auto& item : m_vecCart)
        {
            DeliveryOkDlg::OrderLineItem line;
            //line.strMenuName = CString(item.menuName.c_str());
            line.strMenuName = CA2T(item.menuName.c_str(), CP_UTF8);
            line.nQuantity = item.quantity;
            line.nPrice = item.totalPrice;

            // selectedOptions 순회하여 옵션 문자열 조립
            CString strOpts;
            for (const auto& opt : item.selectedOptions)
            {
                if (!strOpts.IsEmpty()) strOpts += _T(", ");
                CString s;
                CString strOptName = CA2T(opt.optionName.c_str(), CP_UTF8);
                s.Format(_T("%s +%d"), (LPCTSTR)strOptName, opt.optionPrice);
                //s.Format(_T("%s +%d"), CA2T(opt.optionName.c_str(), CP_UTF8), opt.optionPrice);
                strOpts += s;
            }
            line.strOptions = strOpts;
            dlg.m_vecOrderLines.push_back(line);
        }

        ShowWindow(SW_HIDE);
        if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
        else ShowWindow(SW_SHOW);
        return;
    }

    // ── 실제 주문 전송 ────────────────────────────────────────
    int storeID = OrderManager::GetInstance().GetCurrentStoreID();
    std::string addr    = CT2A(strAddr,              CP_UTF8);
    std::string request = CT2A(m_strDeliveryRequest, CP_UTF8);

    // ── 선택된 결제수단 ID 가져오기 ──────────────────────────
    int payMethodID = 0;
    CWnd* pComboWnd = this->GetDlgItem(IDC_COMBO_PAYMENT);
    if (pComboWnd) {
        CComboBox* pCombo = static_cast<CComboBox*>(pComboWnd);
        int sel = pCombo->GetCurSel();
        if (sel >= 0 && sel < (int)m_vecCardIDs.size())
            payMethodID = m_vecCardIDs[sel];
    }

    std::string json = BuildOrderJson(
        m_vecCart, storeID, m_bDelivery,
        addr, request,
        payMethodID,   // 선택된 카드 ID
        usePoint,
        couponID);

    m_bWaiting.store(true);
    this->GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("처리 중..."));

    // OnOrderResponse에서 팝업 구성 시 필요한 값 저장
    m_strLastAddr    = CT2A(strAddr, CP_UTF8);  // CString → std::string 변환
    m_nLastUsePoint  = usePoint;

    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_CREATE_ORDER, json);
}

// ── 주문 서버 응답 ────────────────────────────────────────────
LRESULT CartDlg::OnOrderResponse(WPARAM, LPARAM lParam)
{
    std::unique_ptr<std::string> pBody(reinterpret_cast<std::string*>(lParam));

    m_bWaiting.store(false);
    this->GetDlgItem(IDOK)->EnableWindow(TRUE);

    if (!pBody) return 0;

    try {
        json res = json::parse(*pBody);
        int status = res.value("status", -1);

        if (status == (int)Status::SUCCESS) {
            //std::string oid = res.contains("order_id") ? res["order_id"].get<std::string>() : "0";
            std::string oid = "0";
            if (res.contains("order_id")) {
                if (res["order_id"].is_number())
                    oid = std::to_string(res["order_id"].get<int>()); // 숫자인 경우 변환
                else
                    oid = res["order_id"].get<std::string>(); // 문자열인 경우 그대로
            }

            // ── 주문 완료 팝업에 전달할 정보 구성 ────────────────
            // 가게 이름: OrderManager에서 현재 선택된 가게 찾기
            CString strStoreName;
            int curStoreID = OrderManager::GetInstance().GetCurrentStoreID();
            auto allStores = OrderManager::GetInstance().GetStoresByCategory("전체");
            for (const auto& s : allStores) {
                if (s.storeID == curStoreID) {
                    strStoreName = CA2T(s.storeName.c_str(), CP_UTF8);
                    break;
                }
            }
            if (strStoreName.IsEmpty()) strStoreName = _T("주문 가게");

            CString strOrderList;
            int totalAmt = 0;
            for (const auto& item : m_vecCart) {
                CString name = CA2T(item.menuName.c_str(), CP_UTF8);
                CString line;
                line.Format(_T("%s x%d  %d원\n"), (LPCTSTR)name, item.quantity, item.totalPrice);
                strOrderList += line;
                totalAmt += item.totalPrice;
            }
            //if (m_bDelivery) totalAmt += 3000; // 배달비
            int actualFee = 0;
            if (m_bDelivery && curStoreID != -1) {
                actualFee = OrderManager::GetInstance().GetDeliveryFeeByStore(curStoreID);
            }

            totalAmt += actualFee; // 실제 배달비 합산
            totalAmt -= m_nLastUsePoint; // 사용 포인트 차감 (이것도 반영해야 정확함)

            // 현재 시각
            CTime now = CTime::GetCurrentTime();
            CString strNow = now.Format(_T("%Y-%m-%d %H:%M"));

            // 결제 수단명
            CString strPayLabel;
            CWnd* pComboWnd2 = this->GetDlgItem(IDC_COMBO_PAYMENT);
            if (pComboWnd2) static_cast<CComboBox*>(pComboWnd2)->GetWindowText(strPayLabel);

            //int delivFee2 = m_bDelivery ? 3000 : 0;

            DeliveryOkDlg dlg(this);
            dlg.m_strOrderID = CA2T(oid.c_str(), CP_UTF8);
            int est = res.value("estimated_minutes", 30);
            dlg.m_nEstimatedMinutes = (est > 0) ? est : 30;

            //dlg.m_strOrderID        = CA2T(oid.c_str(), CP_UTF8);
            //dlg.m_nEstimatedMinutes = (est > 0) ? est : 30;
            dlg.m_strStoreName      = strStoreName;
            dlg.m_strOrderList      = strOrderList;   // 구버전 호환용 (빈 m_vecOrderLines 대비)
            dlg.m_nTotalAmount      = totalAmt;
            dlg.m_strDeliveryAddr   = m_bDelivery ? CA2T(m_strLastAddr.c_str(), CP_UTF8) : _T("포장(픽업)");
            dlg.m_strOrderDateTime  = strNow;
            dlg.m_nUsedPoint        = m_nLastUsePoint;
            //dlg.m_nDeliveryFee      = delivFee2;
            dlg.m_strPayMethod      = strPayLabel;
            dlg.m_bDelivery = m_bDelivery;    // ★ 포장/배달 플래그
            dlg.m_nOrderId  = (oid.empty()) ? 0 : std::stoi(oid);  // ★ 사장님 채팅용 order_id
            dlg.m_nDeliveryFee = actualFee; // 실제 배달비 전달

            // ★ 옵션 포함 주문 항목 구성 (온라인 분기)
            for (const auto& item : m_vecCart)
            {
                DeliveryOkDlg::OrderLineItem line;
                line.strMenuName = CA2T(item.menuName.c_str(), CP_UTF8);
                line.nQuantity = item.quantity;
                line.nPrice = item.totalPrice;

                CString strOpts;
                for (const auto& opt : item.selectedOptions) {
                    if (!strOpts.IsEmpty()) strOpts += _T(", ");
                    CString s;
                    CString strOptName = CA2T(opt.optionName.c_str(), CP_UTF8);
                    s.Format(_T("%s +%d"), (LPCTSTR)strOptName, opt.optionPrice);
                    //s.Format(_T("%s +%d"), CA2T(opt.optionName.c_str(), CP_UTF8), opt.optionPrice);
                    strOpts += s;
                }
                line.strOptions = strOpts;
                dlg.m_vecOrderLines.push_back(line);
            }

            OrderManager::GetInstance().ClearCart();
            ShowWindow(SW_HIDE);
            if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
            else ShowWindow(SW_SHOW);
        }
        else {
            UpdateCartUI();
            AfxMessageBox(_T("주문에 실패했습니다."));
        }
    }
    catch (...) {
        UpdateCartUI();
        AfxMessageBox(_T("데이터 처리 중 오류가 발생했습니다."));
    }
    return 0;
}

// ── 옵션 수정 버튼 ────────────────────────────────────────────
void CartDlg::OnBnClickedBtnEditOption()
{
    int n = m_listCart.GetSelectionMark();
    if (n < 0 || n >= (int)m_vecCart.size()) {
        AfxMessageBox(_T("수정할 메뉴를 선택해주세요."), MB_ICONWARNING);
        return;
    }
    CartItem& item = m_vecCart[n];

    OptionChangeDlg dlg(this);
    dlg.m_strMenuName = CA2T(item.menuName.c_str(), CP_UTF8);
    dlg.m_nQuantity = item.quantity;
    dlg.m_nBasePrice = item.basePrice;
    dlg.m_vecOptionGroups = item.optionGroups;

    // 기존 선택 옵션 체크 복원용
    dlg.m_vecPreCheckedOptionIDs.clear();
    for (const auto& sel : item.selectedOptions)
        dlg.m_vecPreCheckedOptionIDs.push_back(sel.optionID);

    // ★ DoModal() 호출 — 여기서 창이 열림
    if (dlg.DoModal() == IDOK)
    {
        if (dlg.m_nQuantity <= 0) {
            // 수량 0 → 장바구니에서 삭제
            m_vecCart.erase(m_vecCart.begin() + n);
        }
        else {
            item.quantity = dlg.m_nQuantity;
            item.selectedOptions.clear();

            // ★ DoModal 반환 후 m_vecResultOptionIDs로 비교 (GetCheck 사용 안 함)
            for (const auto& og : dlg.m_vecOptionGroups) {
                for (const auto& oi : og.items) {
                    bool bChecked = std::find(
                        dlg.m_vecResultOptionIDs.begin(),
                        dlg.m_vecResultOptionIDs.end(),
                        oi.optionID) != dlg.m_vecResultOptionIDs.end();
                    if (bChecked)
                        item.selectedOptions.push_back(oi);
                }
            }
            item.CalculateTotalPrice();
        }
        UpdateCartUI();
    }
}

void CartDlg::OnBnClickedRadioDelivery() { m_bDelivery = true;  UpdateCartUI(); }
void CartDlg::OnBnClickedRadioPickup()   { m_bDelivery = false; UpdateCartUI(); }

void CartDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_CREATE_ORDER);
    CDialogEx::OnCancel();
}

// ── 결제수단 콤보박스 채우기 ──────────────────────────────────
// 서버 연결 시: REQ_GET_PROFILE 로 카드 목록 요청
// 미연결 시: "카드 없음 (직접 결제)" 더미 항목 표시
void CartDlg::LoadPaymentCards()
{
    CWnd* pComboWnd = this->GetDlgItem(IDC_COMBO_PAYMENT);
    if (!pComboWnd) return;
    CComboBox* pCombo = static_cast<CComboBox*>(pComboWnd);
    pCombo->ResetContent();
    m_vecCardIDs.clear();

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인: PaymentDlg에서 등록한 캐시 카드 목록 표시
        if (!g_cachedCards.empty()) {
            for (const auto& c : g_cachedCards) {
                CString label;
                label.Format(_T("%s  %s"), (LPCTSTR)c.alias, (LPCTSTR)c.masked);
                pCombo->AddString(label);
                m_vecCardIDs.push_back(c.id);
            }
        } else {
            pCombo->AddString(_T("카드 미등록 (테스트 결제)"));
            m_vecCardIDs.push_back(0);
        }
        pCombo->SetCurSel(0);
        return;
    }

    // 서버에 카드 목록 요청 (동기식 간이 처리)
    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    std::vector<std::pair<int,CString>> cards; // {id, "별칭 ****-1234"}

    net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
        [&cards, hEvent](uint16_t, const std::string& body) {
            // "payment_methods":[{id,alias,masked},...] 파싱
            std::string arrToken = "\"payment_methods\":[";
            auto ap = body.find(arrToken);
            if (ap != std::string::npos) {
                size_t i = ap + arrToken.size();
                while (i < body.size()) {
                    auto s = body.find('{', i);
                    if (s == std::string::npos) break;
                    int d = 0; size_t e = s;
                    for (; e < body.size(); ++e) {
                        if (body[e] == '{') ++d;
                        else if (body[e] == '}') { if (--d == 0) break; }
                    }
                    std::string obj = body.substr(s, e - s + 1);

                    // id 파싱
                    int cid = 0;
                    {
                        auto p = obj.find("\"id\":");
                        if (p != std::string::npos)
                            try { cid = std::stoi(obj.substr(p + 5)); } catch (...) {}
                    }
                    // alias 파싱
                    std::string alias;
                    {
                        std::string t = "\"alias\":\"";
                        auto p = obj.find(t);
                        if (p != std::string::npos) {
                            p += t.size();
                            auto e2 = obj.find('"', p);
                            if (e2 != std::string::npos) alias = obj.substr(p, e2 - p);
                        }
                    }
                    // masked 파싱
                    std::string masked;
                    {
                        std::string t = "\"masked\":\"";
                        auto p = obj.find(t);
                        if (p != std::string::npos) {
                            p += t.size();
                            auto e2 = obj.find('"', p);
                            if (e2 != std::string::npos) masked = obj.substr(p, e2 - p);
                        }
                    }
                    if (cid > 0) {
                        CString label = CA2T((alias + "  " + masked).c_str(), CP_UTF8);
                        cards.push_back({cid, label});
                    }
                    i = e + 1;
                }
            }
            SetEvent(hEvent);
        });

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCommon::REQ_GET_PROFILE,
                   "{\"request_type\":\"get_cards\"}");

    WaitForSingleObject(hEvent, 3000);
    CloseHandle(hEvent);
    net.UnregisterCallback(CmdCommon::REQ_GET_PROFILE);

    if (cards.empty()) {
        // ★ 수정: 더미 ID 넣지 않고 안내 문구만 표시, SetCurSel 안 함
        pCombo->AddString(_T("등록된 카드가 없습니다. (마이페이지에서 등록)"));
        // m_vecCardIDs는 비워둔 채로 유지 → OnBnClickedOk에서 차단됨
    } else {
        for (const auto& c : cards) {
            pCombo->AddString(c.second);
            m_vecCardIDs.push_back(c.first);
        }
    }
    pCombo->SetCurSel(0);
}

// ================================================================
//  IDC_BTN_ADDR_CHANGE — 배달주소 변경 (주소 관리 다이얼로그)
// ================================================================
void CartDlg::OnBnClickedBtnAddrChange()
{
    AddressDlg dlg(this);
    if (dlg.DoModal() == IDOK && !dlg.m_strSelectedAddr.IsEmpty()) {
        // 선택한 주소를 배달주소 입력란에 자동 입력
        CWnd* pAddr = GetDlgItem(IDC_EDIT_DELIVERY_ADDR);
        if (pAddr) pAddr->SetWindowText(dlg.m_strSelectedAddr);
    }
}
void CartDlg::OnEnChangeEditUsePoint()
{
    // 사용자가 숫자를 타이핑할 때마다 UpdateCartUI를 호출해 금액을 갱신합니다.
    UpdateCartUI();
}
