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

static std::string EscJ(const std::string& s)
{
    std::string o; for(char c:s){ if(c=='"') o+="\\\""; else o+=c; } return o;
}
static std::string BuildOrderJson(const std::vector<CartItem>& cart, int storeID,
    bool isDelivery, const std::string& addr,
    const std::string& request,              // ★ 요청사항
    int payID, int usePoint, int couponID)
{
    std::string method = isDelivery ? "\uBC30\uB2EC" : "\uD3EC\uC7A5";
    std::string j = "{";
    j += "\"store_id\":"          + std::to_string(storeID) + ",";
    j += "\"delivery_method\":\"" + method + "\",";
    if (isDelivery) {
        j += "\"delivery_address\":\"" + EscJ(addr) + "\",";
        j += "\"delivery_request\":\"" + EscJ(request) + "\","; // ★
    }
    j += "\"payment_method_id\":" + std::to_string(payID) + ",";
    j += "\"use_point\":"         + std::to_string(usePoint) + ",";
    j += "\"coupon_id\":"         + std::to_string(couponID) + ",";
    j += "\"items\":[";
    for (size_t i = 0; i < cart.size(); ++i) {
        if (i) j += ",";
        const CartItem& item = cart[i];
        j += "{\"menu_id\":"        + std::to_string(item.menuID) + ",";
        j += "\"menu_name\":\""     + EscJ(item.menuName) + "\",";
        j += "\"price_at_order\":"  + std::to_string(item.basePrice) + ",";
        j += "\"quantity\":"        + std::to_string(item.quantity) + ",";
        j += "\"options\":[";
        for (size_t k = 0; k < item.selectedOptions.size(); ++k) {
            if (k) j += ",";
            const OptionItem& opt = item.selectedOptions[k];
            j += "{\"option_id\":"    + std::to_string(opt.optionID) + ",";
            j += "\"option_name\":\"" + EscJ(opt.optionName) + "\",";
            j += "\"option_price\":"  + std::to_string(opt.optionPrice) + "}";
        }
        j += "]}";
    }
    j += "]}";
    return j;
}
static std::string ParseOID(const std::string& b)
{
    std::string t="\"order_id\":\""; auto p=b.find(t);
    if(p==std::string::npos) return "";
    p+=t.size(); auto e=b.find('"',p);
    return (e==std::string::npos)?"":b.substr(p,e-p);
}
static int ParseStat(const std::string& b)
{
    std::string t="\"status\":"; auto p=b.find(t);
    if(p==std::string::npos) return -1;
    try{return std::stoi(b.substr(p+t.size()));}catch(...){return -1;}
}
static int ParseEst(const std::string& b)
{
    std::string t="\"estimated_minutes\":"; auto p=b.find(t);
    if(p==std::string::npos) return 30;
    try{return std::stoi(b.substr(p+t.size()));}catch(...){return 30;}
}

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
    ModifyStyle(WS_CAPTION, 0); CenterWindow();

    m_listCart.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listCart.InsertColumn(0, _T("메뉴명"),  LVCFMT_LEFT,  150);
    m_listCart.InsertColumn(1, _T("가격"),    LVCFMT_RIGHT,  90);
    m_listCart.InsertColumn(2, _T("수량"),    LVCFMT_CENTER, 40);
    m_listCart.InsertColumn(3, _T("옵션"),    LVCFMT_LEFT,  110);

    m_bDelivery = true;
    CheckDlgButton(IDC_RADIO_DELIVERY, BST_CHECKED);
    CheckDlgButton(IDC_RADIO_PICKUP,   BST_UNCHECKED);

    // 배달 요청사항 힌트 텍스트
    CWnd* pReq = this->GetDlgItem(IDC_EDIT_DELIVERY_REQUEST);
    if (pReq) {
        // EDITTEXT에 힌트(cue banner) 설정
        ::SendMessage(pReq->GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                      (LPARAM)_T("예: 문 앞에 놓아주세요 / 벨 누르지 마세요"));
    }

    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_CREATE_ORDER,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_ORDER_RESPONSE, 0, (LPARAM)p);
        });

    m_vecCart = OrderManager::GetInstance().GetCartItems();
    UpdateCartUI();
    return TRUE;
}

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
        m_listCart.SetItemText(r,1,pr);
        m_listCart.SetItemText(r,2,q);
        m_listCart.SetItemText(r,3,op);
        total += m_vecCart[i].totalPrice;
    }
    int fee   = m_bDelivery ? 3000 : 0;
    int final = total + fee;
    CString s;
    s.Format(_T("%d원"), total); SetDlgItemText(IDC_STATIC_ORDER_PRICE, s);
    s.Format(_T("%d원"), fee);   SetDlgItemText(IDC_STATIC_DELIVERY_FEE, s);
    s.Format(_T("%d원"), final); SetDlgItemText(IDC_STATIC_TOTAL_PRICE, s);
    s.Format(_T("주문하기 (%d원)"), final); SetDlgItemText(IDOK, s);

    // 배달 요청사항 레이블/에디트 표시여부
    CWnd* pLbl = this->GetDlgItem(IDC_STATIC_REQUEST_LABEL);
    CWnd* pEdt = this->GetDlgItem(IDC_EDIT_DELIVERY_REQUEST);
    if (pLbl) pLbl->ShowWindow(m_bDelivery ? SW_SHOW : SW_HIDE);
    if (pEdt) pEdt->ShowWindow(m_bDelivery ? SW_SHOW : SW_HIDE);
}

void CartDlg::OnBnClickedOk()
{
    if (m_bWaiting.load()) return;
    if (m_vecCart.empty()) {
        AfxMessageBox(_T("장바구니가 비어있습니다."), MB_ICONWARNING); return;
    }

    // ── 배달 요청사항 읽기 ───────────────────────────────────
    CWnd* pReq = this->GetDlgItem(IDC_EDIT_DELIVERY_REQUEST);
    if (pReq) pReq->GetWindowText(m_strDeliveryRequest);
    m_strDeliveryRequest.Trim();

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인 테스트
        std::string dummy;
        OrderManager::GetInstance().ProcessOrder("card_default", 0, "", dummy);
        DeliveryOkDlg dlg(this);
        dlg.m_strOrderID = _T("TEST-0001");
        dlg.m_nEstimatedMinutes = 30;
        ShowWindow(SW_HIDE);
        if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
        else ShowWindow(SW_SHOW);
        return;
    }

    int storeID  = OrderManager::GetInstance().GetCurrentStoreID();
    std::string request = CT2A(m_strDeliveryRequest, CP_UTF8);
    std::string json = BuildOrderJson(m_vecCart, storeID, m_bDelivery,
                                      "", request, 1, 0, 0);

    m_bWaiting.store(true);
    this->GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("처리 중..."));
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_CREATE_ORDER, json);
}

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
        OrderManager::GetInstance().ClearCart();
        DeliveryOkDlg dlg(this);
        dlg.m_strOrderID        = CA2T(oid.c_str(), CP_UTF8);
        dlg.m_nEstimatedMinutes = est;
        ShowWindow(SW_HIDE);
        if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
        else ShowWindow(SW_SHOW);
    } else {
        delete pBody;
        UpdateCartUI();
        AfxMessageBox(_T("주문에 실패했습니다. 잠시 후 다시 시도해주세요."), MB_ICONERROR);
    }
    return 0;
}

void CartDlg::OnBnClickedBtnEditOption()
{
    int n = m_listCart.GetSelectionMark();
    if (n < 0 || n >= (int)m_vecCart.size()) {
        AfxMessageBox(_T("수정할 메뉴를 선택해주세요."), MB_ICONWARNING); return;
    }
    CartItem& item = m_vecCart[n];
    OptionChangeDlg dlg(this);
    dlg.m_strMenuName = CA2T(item.menuName.c_str(), CP_UTF8);
    dlg.m_nQuantity   = item.quantity;
    dlg.m_nBasePrice  = item.basePrice;
    if (dlg.DoModal() == IDOK) {
        if (dlg.m_nQuantity <= 0)
            m_vecCart.erase(m_vecCart.begin() + n);
        else {
            item.quantity = dlg.m_nQuantity;
            item.CalculateTotalPrice();
        }
        UpdateCartUI();
    }
}
void CartDlg::OnBnClickedRadioDelivery() { m_bDelivery=true;  UpdateCartUI(); }
void CartDlg::OnBnClickedRadioPickup()   { m_bDelivery=false; UpdateCartUI(); }
void CartDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_CREATE_ORDER);
    CDialogEx::OnCancel();
}
