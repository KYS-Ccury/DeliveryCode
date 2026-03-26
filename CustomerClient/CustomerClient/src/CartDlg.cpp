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
#include "DeliveryOkDlg.h"
#include "OptionChangeDlg.h"
#include "OrderManager.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "PaymentDlg.h"
#include "common/header/Types.h"

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
    const std::string& deliveryAddr,   // 배달 주소
    const std::string& request,        // 요청사항
    int payMethodID,                   // 결제 수단 ID (0이면 기본)
    int usePoint,                      // 포인트 사용액
    int couponID)                      // 쿠폰 ID (0이면 미사용)
{
    std::string method = isDelivery
        ? "\uBC30\uB2EC"   // "배달" UTF-8
        : "\uD3EC\uC7A5";  // "포장" UTF-8
    std::string j = "{";
    j += "\"store_id\":"           + std::to_string(storeID)     + ",";
    j += "\"delivery_method\":\"" + method                       + "\",";
    if (isDelivery) {
        j += "\"delivery_address\":\"" + EscJ(deliveryAddr) + "\",";
        j += "\"delivery_request\":\"" + EscJ(request)      + "\",";
    }
    j += "\"payment_method_id\":"  + std::to_string(payMethodID) + ",";
    j += "\"use_point\":"          + std::to_string(usePoint)    + ",";
    j += "\"coupon_id\":"          + std::to_string(couponID)    + ",";
    j += "\"items\":[";
    for (size_t i = 0; i < cart.size(); ++i) {
        if (i) j += ",";
        const CartItem& item = cart[i];
        j += "{\"menu_id\":"    + std::to_string(item.menuID)    + ",";
        j += "\"name\":\""      + EscJ(item.menuName)            + "\",";
        j += "\"price\":"       + std::to_string(item.basePrice) + ",";  // 서버: price
        j += "\"qty\":"         + std::to_string(item.quantity)  + ",";  // 서버: qty
        j += "\"options\":[";
        for (size_t k = 0; k < item.selectedOptions.size(); ++k) {
            if (k) j += ",";
            const OptionItem& opt = item.selectedOptions[k];
            j += "{\"option_item_id\":"  + std::to_string(opt.optionID)   + ",";  // 서버: option_item_id
            j += "\"option_name\":\"" + EscJ(opt.optionName)            + "\",";
            j += "\"extra_price\":"   + std::to_string(opt.optionPrice) + "}";   // 서버: extra_price
        }
        j += "]}";
    }
    j += "]}";
    return j;
}

// ── 응답 파싱 헬퍼 ────────────────────────────────────────────
static std::string ParseOID(const std::string& b)
{
    // order_id 가 숫자인 경우: "order_id":123
    std::string t1 = "\"order_id\":\"";
    auto p1 = b.find(t1);
    if (p1 != std::string::npos) {
        p1 += t1.size();
        auto e = b.find('"', p1);
        return (e == std::string::npos) ? "" : b.substr(p1, e - p1);
    }
    std::string t2 = "\"order_id\":";
    auto p2 = b.find(t2);
    if (p2 != std::string::npos) {
        p2 += t2.size();
        auto e = b.find_first_of(",}", p2);
        return (e == std::string::npos) ? "" : b.substr(p2, e - p2);
    }
    return "";
}
static int ParseStat(const std::string& b)
{
    std::string t = "\"status\":";
    auto p = b.find(t);
    if (p == std::string::npos) return -1;
    try { return std::stoi(b.substr(p + t.size())); } catch (...) { return -1; }
}
static int ParseEst(const std::string& b)
{
    std::string t = "\"estimated_minutes\":";
    auto p = b.find(t);
    if (p == std::string::npos) return 30;
    try { return std::stoi(b.substr(p + t.size())); } catch (...) { return 30; }
}

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
    ON_BN_CLICKED(IDC_BTN_BACK,        &CartDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_EDIT_OPTION, &CartDlg::OnBnClickedBtnEditOption)
    ON_BN_CLICKED(IDC_RADIO_DELIVERY,  &CartDlg::OnBnClickedRadioDelivery)
    ON_BN_CLICKED(IDC_RADIO_PICKUP,    &CartDlg::OnBnClickedRadioPickup)
    ON_MESSAGE(WM_ORDER_RESPONSE,      &CartDlg::OnOrderResponse)
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
    CWnd* pAddr = this->GetDlgItem(IDC_EDIT_DELIVERY_ADDR);
    if (pAddr) {
        // 서버에서 프로필 조회 시 저장해 둔 주소가 있으면 자동 입력
        // (현재 AuthManager는 주소를 별도 저장하지 않으므로 빈 값)
        pAddr->SetWindowText(_T(""));
        // 힌트 텍스트
        ::SendMessage(pAddr->GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                      (LPARAM)_T("배달 받을 주소를 입력하세요"));
    }

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
    }

    int fee   = m_bDelivery ? 3000 : 0;
    int final_ = total + fee;

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
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    m_bWaiting.store(false);
    this->GetDlgItem(IDOK)->EnableWindow(TRUE);

    if (!pBody) return 0;

    int status = ParseStat(*pBody);

    if (status == (int)Status::SUCCESS) {
        std::string oid = ParseOID(*pBody);
        int est         = ParseEst(*pBody);
        delete pBody;

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

        // 주문 메뉴 목록 문자열 구성
        //CString strOrderList;
        //for (const auto& item : m_vecCart) {
        //    CString name = CA2T(item.menuName.c_str(), CP_UTF8);
        //    CString line;
        //    line.Format(_T("%s x%d  %d원\n"),
        //        (LPCTSTR)name,
        //        item.quantity,
        //        item.totalPrice);
        //    strOrderList += line;
        //    totalAmt += item.totalPrice;
        //}

        CString strOrderList;
        int totalAmt = 0;
        for (const auto& item : m_vecCart) {
            CString name = CA2T(item.menuName.c_str(), CP_UTF8);
            CString line;
            line.Format(_T("%s x%d  %d원\n"), (LPCTSTR)name, item.quantity, item.totalPrice);
            strOrderList += line;
            totalAmt += item.totalPrice;
        }
        if (m_bDelivery) totalAmt += 3000; // 배달비


        OrderManager::GetInstance().ClearCart();

        // 현재 시각
        CTime now = CTime::GetCurrentTime();
        CString strNow = now.Format(_T("%Y-%m-%d %H:%M"));

        // 결제 수단명
        CString strPayLabel;
        CWnd* pComboWnd2 = this->GetDlgItem(IDC_COMBO_PAYMENT);
        if (pComboWnd2) static_cast<CComboBox*>(pComboWnd2)->GetWindowText(strPayLabel);

        int delivFee2 = m_bDelivery ? 3000 : 0;

        DeliveryOkDlg dlg(this);
        dlg.m_strOrderID        = CA2T(oid.c_str(), CP_UTF8);
        dlg.m_nEstimatedMinutes = (est > 0) ? est : 30;
        dlg.m_strStoreName      = strStoreName;
        dlg.m_strOrderList      = strOrderList;   // 구버전 호환용 (빈 m_vecOrderLines 대비)
        dlg.m_nTotalAmount      = totalAmt;
        dlg.m_strDeliveryAddr   = m_bDelivery ? CA2T(m_strLastAddr.c_str(), CP_UTF8) : _T("포장(픽업)");
        dlg.m_strOrderDateTime  = strNow;
        dlg.m_nUsedPoint        = m_nLastUsePoint;
        dlg.m_nDeliveryFee      = delivFee2;
        dlg.m_strPayMethod      = strPayLabel;
        dlg.m_bDelivery = m_bDelivery;    // ★ 포장/배달 플래그

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

        ShowWindow(SW_HIDE);
        if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
        else ShowWindow(SW_SHOW);

    } else {
        delete pBody;
        UpdateCartUI();  // 버튼 텍스트 복구
        AfxMessageBox(_T("주문에 실패했습니다.\n잠시 후 다시 시도해주세요."), MB_ICONERROR);
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
    dlg.m_strMenuName      = CA2T(item.menuName.c_str(), CP_UTF8);
    dlg.m_nQuantity        = item.quantity;
    dlg.m_nBasePrice       = item.basePrice;


    // 해당 메뉴의 옵션 그룹 전달
    // (OrderManager에서 캐시된 MenuInfo에서 찾기)
    //auto menus = OrderManager::GetInstance().GetMenuData(
    //    OrderManager::GetInstance().GetCurrentStoreID());
    //for (const auto& mi : menus) {
    //    if (mi.menuID == item.menuID) {
    //        dlg.m_vecOptionGroups = mi.optionGroups;
    //        break;
    //    }
    //}

    // ★ CartItem에 저장된 optionGroups 직접 사용 (캐시 조회 불필요)
    dlg.m_vecOptionGroups = item.optionGroups;

    // ★ 기존 선택 옵션 ID 목록 → 체크 상태 복원용
    dlg.m_vecPreCheckedOptionIDs.clear();
    for (const auto& sel : item.selectedOptions)
        dlg.m_vecPreCheckedOptionIDs.push_back(sel.optionID);

    if (dlg.DoModal() == IDOK) {
        if (dlg.m_nQuantity <= 0) {
            // 수량 0 → 장바구니에서 삭제
            m_vecCart.erase(m_vecCart.begin() + n);
        } else {
            item.quantity = dlg.m_nQuantity;

            // ── 체크된 옵션으로 selectedOptions 갱신 ─────────
            item.selectedOptions.clear();
            int row = 0;
            for (const auto& og : dlg.m_vecOptionGroups) {
                for (const auto& oi : og.items) {
                    if (dlg.m_listOptions.GetCheck(row))
                        item.selectedOptions.push_back(oi);
                    row++;
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
