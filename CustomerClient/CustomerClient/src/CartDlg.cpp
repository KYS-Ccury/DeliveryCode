// ================================================================
//  CartDlg.cpp  ─  장바구니 / 주문 요청 화면 (수정본)
//
//  resource.h 기준 사용 IDC:
//    IDC_LIST_CART          1411
//    IDC_BTN_BACK           1400
//    IDC_RADIO_DELIVERY     1401
//    IDC_RADIO_PICKUP       1402
//    IDC_BTN_EDIT_OPTION    1403
//    IDC_COMBO_PAYMENT      1405
//    IDC_STATIC_ORDER_PRICE 1408
//    IDC_STATIC_DELIVERY_FEE 1409
//    IDC_STATIC_TOTAL_PRICE 1410
//
//  [수정사항]
//    - IDC_BTN_DELETE_ITEM 제거 (resource.h 미정의) → 삭제 기능은
//      옵션 편집 팝업(OptionChangeDlg)에서 수량 0으로 처리
//    - DDX_Radio 제거 → CheckDlgButton 으로 직접 제어
//    - PaymentCard 재정의 제거 (헤더에서만 정의)
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
#include "common/header/Types.h"

#define WM_ORDER_RESPONSE (WM_USER + 130)

// ── JSON 빌드 헬퍼 ────────────────────────────────────────────
static std::string EscapeJson(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else                out += c;
    }
    return out;
}

static std::string BuildOrderJson(const std::vector<CartItem>& cart,
                                   int storeID,
                                   bool isDelivery,
                                   const std::string& address,
                                   int paymentMethodID,
                                   int usePoint,
                                   int couponID)
{
    std::string method = isDelivery ? "\uBC30\uB2EC" : "\uD3EC\uC7A5";  // 배달 / 포장
    std::string json = "{";
    json += "\"store_id\":"          + std::to_string(storeID) + ",";
    json += "\"delivery_method\":\"" + method + "\",";
    if (isDelivery)
        json += "\"delivery_address\":\"" + EscapeJson(address) + "\",";
    json += "\"payment_method_id\":" + std::to_string(paymentMethodID) + ",";
    json += "\"use_point\":"         + std::to_string(usePoint) + ",";
    json += "\"coupon_id\":"         + std::to_string(couponID) + ",";
    json += "\"items\":[";

    for (size_t i = 0; i < cart.size(); ++i) {
        const CartItem& item = cart[i];
        if (i > 0) json += ",";
        json += "{";
        json += "\"menu_id\":"        + std::to_string(item.menuID) + ",";
        json += "\"menu_name\":\""    + EscapeJson(item.menuName) + "\",";
        json += "\"price_at_order\":" + std::to_string(item.basePrice) + ",";
        json += "\"quantity\":"       + std::to_string(item.quantity) + ",";
        json += "\"options\":[";
        for (size_t j = 0; j < item.selectedOptions.size(); ++j) {
            const OptionItem& opt = item.selectedOptions[j];
            if (j > 0) json += ",";
            json += "{";
            json += "\"option_id\":"     + std::to_string(opt.optionID) + ",";
            json += "\"option_name\":\"" + EscapeJson(opt.optionName) + "\",";
            json += "\"option_price\":"  + std::to_string(opt.optionPrice);
            json += "}";
        }
        json += "]}";
    }
    json += "]}";
    return json;
}

static std::string ParseOrderID(const std::string& body)
{
    std::string token = "\"order_id\":\"";
    auto pos = body.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = body.find('"', pos);
    return (end == std::string::npos) ? "" : body.substr(pos, end - pos);
}
static int ParseStatus(const std::string& body)
{
    std::string token = "\"status\":";
    auto pos = body.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(body.substr(pos + token.size())); } catch (...) { return -1; }
}
static int ParseEstimated(const std::string& body)
{
    std::string token = "\"estimated_minutes\":";
    auto pos = body.find(token);
    if (pos == std::string::npos) return 30;
    try { return std::stoi(body.substr(pos + token.size())); } catch (...) { return 30; }
}

// =================================================================

IMPLEMENT_DYNAMIC(CartDlg, CDialogEx)

CartDlg::CartDlg(CWnd* pParent)
    : CDialogEx(IDD_CART_DLG, pParent)
    , m_bDelivery(true)
    , m_bWaiting(false)
{}
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

    // ── 리스트 컬럼 설정 ──────────────────────────────────────
    m_listCart.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listCart.InsertColumn(0, _T("메뉴명"),  LVCFMT_LEFT,   150);
    m_listCart.InsertColumn(1, _T("가격"),    LVCFMT_RIGHT,  100);
    m_listCart.InsertColumn(2, _T("수량"),    LVCFMT_CENTER,  50);
    m_listCart.InsertColumn(3, _T("옵션"),    LVCFMT_LEFT,   120);

    // ── 기본값: 배달 선택 ─────────────────────────────────────
    m_bDelivery = true;
    CheckDlgButton(IDC_RADIO_DELIVERY, BST_CHECKED);
    CheckDlgButton(IDC_RADIO_PICKUP,   BST_UNCHECKED);

    // ── 서버 주문 응답 콜백 등록 ─────────────────────────────
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_CREATE_ORDER,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            PostMessage(WM_ORDER_RESPONSE, 0, (LPARAM)pBody);
        });

    // ── 카트 로드 및 UI 갱신 ─────────────────────────────────
    m_vecCart = OrderManager::GetInstance().GetCartItems();
    UpdateCartUI();

    return TRUE;
}

// ── 카트 UI 갱신 ─────────────────────────────────────────────
void CartDlg::UpdateCartUI()
{
    m_listCart.DeleteAllItems();
    int total = 0;

    for (int i = 0; i < (int)m_vecCart.size(); ++i) {
        m_vecCart[i].CalculateTotalPrice();

        CString strName  = CA2T(m_vecCart[i].menuName.c_str(), CP_UTF8);
        CString strPrice; strPrice.Format(_T("%d\uC6D0"), m_vecCart[i].totalPrice);
        CString strQty;   strQty.Format(_T("%d"), m_vecCart[i].quantity);

        // 선택된 옵션 요약
        CString strOpts;
        for (const auto& opt : m_vecCart[i].selectedOptions) {
            if (!strOpts.IsEmpty()) strOpts += _T(", ");
            strOpts += CA2T(opt.optionName.c_str(), CP_UTF8);
        }

        int nRow = m_listCart.InsertItem(i, strName);
        m_listCart.SetItemText(nRow, 1, strPrice);
        m_listCart.SetItemText(nRow, 2, strQty);
        m_listCart.SetItemText(nRow, 3, strOpts);

        total += m_vecCart[i].totalPrice;
    }

    int fee   = m_bDelivery ? 3000 : 0;
    int final = total + fee;

    CString s;
    s.Format(_T("%d\uC6D0"), total); SetDlgItemText(IDC_STATIC_ORDER_PRICE, s);
    s.Format(_T("%d\uC6D0"), fee);   SetDlgItemText(IDC_STATIC_DELIVERY_FEE, s);
    s.Format(_T("%d\uC6D0"), final); SetDlgItemText(IDC_STATIC_TOTAL_PRICE, s);
    s.Format(_T("\uC8FC\uBB38\uD558\uAE30 (%d\uC6D0)"), final);
    SetDlgItemText(IDOK, s);
}

// ── 주문하기 버튼 ─────────────────────────────────────────────
void CartDlg::OnBnClickedOk()
{
    if (m_bWaiting) return;
    if (m_vecCart.empty()) {
        AfxMessageBox(_T("\uC7A5\uBC14\uAD6C\uB2C8\uAC00 \uBE44\uC5B4\uC788\uC2B5\uB2C8\uB2E4."), MB_ICONWARNING);
        return;
    }

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 서버 없을 때 로컬 처리
        std::string dummy;
        OrderManager::GetInstance().ProcessOrder("card_default", 0, "", dummy);

        DeliveryOkDlg dlg(this);
        dlg.m_strOrderID        = _T("TEST-0001");
        dlg.m_nEstimatedMinutes = 30;
        this->ShowWindow(SW_HIDE);
        if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
        else this->ShowWindow(SW_SHOW);
        return;
    }

    // ── 서버 전송 ─────────────────────────────────────────────
    int storeID     = OrderManager::GetInstance().GetCurrentStoreID();
    int payMethodID = 1;   // TODO: 카드 선택 연동
    int usePoint    = 0;
    int couponID    = 0;
    std::string address = "";  // TODO: AuthManager::GetCurrentUser().address

    std::string json = BuildOrderJson(
        m_vecCart, storeID, m_bDelivery, address,
        payMethodID, usePoint, couponID);

    m_bWaiting = true;
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("\uC8FC\uBB38 \uCC98\uB9AC \uC911..."));

    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_CREATE_ORDER, json);
}

// ── 주문 서버 응답 처리 (UI 스레드) ──────────────────────────
LRESULT CartDlg::OnOrderResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    m_bWaiting = false;
    GetDlgItem(IDOK)->EnableWindow(TRUE);

    int status = ParseStatus(*pBody);

    if (status == (int)Status::SUCCESS) {
        std::string orderID  = ParseOrderID(*pBody);
        int estimatedMin     = ParseEstimated(*pBody);

        OrderManager::GetInstance().ClearCart();
        delete pBody;

        DeliveryOkDlg dlg(this);
        dlg.m_strOrderID        = CA2T(orderID.c_str(), CP_UTF8);
        dlg.m_nEstimatedMinutes = estimatedMin;
        this->ShowWindow(SW_HIDE);
        if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
        else this->ShowWindow(SW_SHOW);
    } else {
        delete pBody;
        UpdateCartUI();
        AfxMessageBox(_T("\uC8FC\uBB38\uC5D0 \uC2E4\uD328\uD588\uC2B5\uB2C8\uB2E4.\n\uC7A0\uC2DC \uD6C4 \uB2E4\uC2DC \uC2DC\uB3C4\uD574\uC8FC\uC138\uC694."), MB_ICONERROR);
    }
    return 0;
}

// ── 옵션/수량 변경 버튼 ───────────────────────────────────────
void CartDlg::OnBnClickedBtnEditOption()
{
    int nIndex = m_listCart.GetSelectionMark();
    if (nIndex < 0 || nIndex >= (int)m_vecCart.size()) {
        AfxMessageBox(_T("\uC218\uC815\uD560 \uBA54\uB274\uB97C \uC120\uD0DD\uD574\uC8FC\uC138\uC694."), MB_ICONWARNING);
        return;
    }
    CartItem& item = m_vecCart[nIndex];
    OptionChangeDlg dlg(this);
    dlg.m_strMenuName = CA2T(item.menuName.c_str(), CP_UTF8);
    dlg.m_nQuantity   = item.quantity;
    dlg.m_nBasePrice  = item.basePrice;

    if (dlg.DoModal() == IDOK) {
        if (dlg.m_nQuantity <= 0) {
            // 수량 0이면 항목 삭제
            m_vecCart.erase(m_vecCart.begin() + nIndex);
        } else {
            item.quantity = dlg.m_nQuantity;
            item.CalculateTotalPrice();
        }
        UpdateCartUI();
    }
}

// ── 배달/포장 라디오 ─────────────────────────────────────────
void CartDlg::OnBnClickedRadioDelivery()
{
    m_bDelivery = true;
    UpdateCartUI();
}
void CartDlg::OnBnClickedRadioPickup()
{
    m_bDelivery = false;
    UpdateCartUI();
}

void CartDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_CREATE_ORDER);
    CDialogEx::OnCancel();
}
