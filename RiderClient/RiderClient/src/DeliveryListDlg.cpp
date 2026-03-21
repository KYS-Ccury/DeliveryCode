// DeliveryListDlg.cpp - Dispatch list with JSON protocol
#include "pch.h"
#include "DeliveryListDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "DispatchDlg.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(DeliveryListDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DeliveryListDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_REFRESH,      &DeliveryListDlg::OnBtnRefresh)
    ON_BN_CLICKED(IDC_BTN_ACCEPT_ITEM,  &DeliveryListDlg::OnBtnAcceptItem)
    ON_BN_CLICKED(IDC_BTN_REJECT_ITEM,  &DeliveryListDlg::OnBtnRejectItem)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_ORDERS, &DeliveryListDlg::OnDblclkList)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_LIST, &DeliveryListDlg::OnTabSelChange)
    ON_MESSAGE(WM_SOCKET_RECV,          &DeliveryListDlg::OnSocketRecv)
    ON_WM_TIMER()
END_MESSAGE_MAP()

DeliveryListDlg::DeliveryListDlg(CWnd* pParent) : CDialogEx(IDD_DELIVERY_LIST_DLG, pParent) {}
DeliveryListDlg::~DeliveryListDlg() {}

void DeliveryListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_ORDERS, m_listOrders);
    DDX_Control(pDX, IDC_TAB_LIST,    m_tabList);
}

BOOL DeliveryListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    TC_ITEM ti = {};
    ti.mask = TCIF_TEXT;
    ti.pszText = const_cast<LPTSTR>(_T("배차 대기"));
    m_tabList.InsertItem(0, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("이전 내역"));
    m_tabList.InsertItem(1, &ti);

    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());
    InitListCtrl();
    RefreshCurrentList();
    SetTimer(1, 1000, nullptr);
    return TRUE;
}

void DeliveryListDlg::InitListCtrl()
{
    m_listOrders.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listOrders.DeleteAllItems();
    while (m_listOrders.DeleteColumn(0)) {}

    if (m_tabList.GetCurSel() == 0) {
        m_listOrders.InsertColumn(0, _T("경과"),  LVCFMT_LEFT,  70);
        m_listOrders.InsertColumn(1, _T("가게명"),    LVCFMT_LEFT, 110);
        m_listOrders.InsertColumn(2, _T("픽업주소"),   LVCFMT_LEFT, 140);
        m_listOrders.InsertColumn(3, _T("배달주소"),     LVCFMT_LEFT, 140);
        m_listOrders.InsertColumn(4, _T("배달료"),      LVCFMT_RIGHT, 70);
    } else {
        m_listOrders.InsertColumn(0, _T("날짜/시간"), LVCFMT_LEFT, 120);
        m_listOrders.InsertColumn(1, _T("주문코드"), LVCFMT_LEFT,  90);
        m_listOrders.InsertColumn(2, _T("가게명"),     LVCFMT_LEFT, 110);
        m_listOrders.InsertColumn(3, _T("배달료"),       LVCFMT_RIGHT, 70);
        m_listOrders.InsertColumn(4, _T("상태"),    LVCFMT_LEFT,  60);
    }
}

// CMD_RIDER_ORDER_LIST (400), body="{}"
void DeliveryListDlg::RefreshCurrentList()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_ORDER_LIST, "{}");
    if (!bSent) {
        m_items.RemoveAll();
        OrderListItem i1; i1.orderId=1001; i1.storeName=_T("상무치킨");
        i1.pickupAddr=_T("상무대로 123"); i1.destAddr=_T("치평동 456");
        i1.deliveryFee=3500; i1.elapsedSec=130; m_items.Add(i1);
        OrderListItem i2; i2.orderId=1002; i2.storeName=_T("피자헛 상무점");
        i2.pickupAddr=_T("상무중앙로 45"); i2.destAddr=_T("마륵동 789");
        i2.deliveryFee=4000; i2.elapsedSec=330; m_items.Add(i2);
        PopulateList();
    }
}

void DeliveryListDlg::PopulateList()
{
    m_listOrders.DeleteAllItems();
    for (INT_PTR i = 0; i < m_items.GetSize(); i++) {
        const OrderListItem& item = m_items[i];
        int m = item.elapsedSec / 60, s = item.elapsedSec % 60;
        CString elapsed; elapsed.Format(_T("%d분 %02d초"), m, s);
        CString fee; fee.Format(_T("%d원"), item.deliveryFee);
        int nRow = m_listOrders.InsertItem((int)i, elapsed);
        m_listOrders.SetItemText(nRow, 1, item.storeName);
        m_listOrders.SetItemText(nRow, 2, item.pickupAddr);
        m_listOrders.SetItemText(nRow, 3, item.destAddr);
        m_listOrders.SetItemText(nRow, 4, fee);
        m_listOrders.SetItemData(nRow, (DWORD_PTR)item.orderId);
    }
}

// CMD_RIDER_MY_LIST (405), body="{}"
void DeliveryListDlg::PopulateHistoryList()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST, "{}");
    if (!bSent) {
        m_listOrders.DeleteAllItems();
        struct { LPCTSTR dt; LPCTSTR code; LPCTSTR store; int fee; } dummy[] = {
            { _T("03/20 19:35"), _T("ORD000001"), _T("상무치킨"), 3500 },
            { _T("03/20 17:55"), _T("ORD000002"), _T("피자헛 상무점"), 4000 },
            { _T("03/20 15:10"), _T("ORD000003"), _T("BurgerKing Sangmu"), 3000 },
        };
        for (int i = 0; i < 3; i++) {
            CString fee; fee.Format(_T("%d원"), dummy[i].fee);
            int nRow = m_listOrders.InsertItem(i, dummy[i].dt);
            m_listOrders.SetItemText(nRow, 1, dummy[i].code);
            m_listOrders.SetItemText(nRow, 2, dummy[i].store);
            m_listOrders.SetItemText(nRow, 3, fee);
            m_listOrders.SetItemText(nRow, 4, _T("완료"));
        }
    }
}

void DeliveryListDlg::UpdateElapsedTime()
{
    if (m_tabList.GetCurSel() != 0) return;
    for (INT_PTR i = 0; i < m_items.GetSize(); i++) {
        m_items[i].elapsedSec++;
        int m = m_items[i].elapsedSec / 60, s = m_items[i].elapsedSec % 60;
        CString elapsed; elapsed.Format(_T("%d분 %02d초"), m, s);
        m_listOrders.SetItemText((int)i, 0, elapsed);
    }
}

void DeliveryListDlg::OnBtnRefresh()
{
    if (m_tabList.GetCurSel() == 0) { m_items.RemoveAll(); RefreshCurrentList(); }
    else PopulateHistoryList();
}

void DeliveryListDlg::OnBtnAcceptItem()
{
    int nSel = m_listOrders.GetNextItem(-1, LVNI_SELECTED);
    if (nSel < 0) { MessageBox(_T("항목을 선택하세요."), _T("알림"), MB_OK); return; }
    if (nSel >= (int)m_items.GetSize()) return;
    const OrderListItem& item = m_items[nSel];
    CString pushData;
    pushData.Format(_T("700|%d|%s|%s|%s|%d"),
                    item.orderId, static_cast<LPCTSTR>(item.storeName),
                    static_cast<LPCTSTR>(item.pickupAddr),
                    static_cast<LPCTSTR>(item.destAddr), item.deliveryFee);
    DispatchDlg dlg(pushData, this);
    if (dlg.DoModal() == IDOK) {
        m_items.RemoveAt(nSel); PopulateList(); EndDialog(IDOK);
    }
}

void DeliveryListDlg::OnBtnRejectItem()
{
    int nSel = m_listOrders.GetNextItem(-1, LVNI_SELECTED);
    if (nSel < 0 || nSel >= (int)m_items.GetSize()) return;
    json req; req["order_id"] = m_items[nSel].orderId; req["reason"] = "MANUAL";
    AppContext::Get().socket.SendPacket(CMD_RIDER_REJECT, req.dump());
    m_items.RemoveAt(nSel); PopulateList();
}

void DeliveryListDlg::OnDblclkList(NMHDR*, LRESULT* pResult)
{
    if (m_tabList.GetCurSel() == 0) OnBtnAcceptItem();
    *pResult = 0;
}

void DeliveryListDlg::OnTabSelChange(NMHDR*, LRESULT* pResult)
{
    InitListCtrl();
    if (m_tabList.GetCurSel() == 0) RefreshCurrentList();
    else PopulateHistoryList();
    *pResult = 0;
}

void DeliveryListDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) UpdateElapsedTime();
    CDialogEx::OnTimer(nIDEvent);
}

void DeliveryListDlg::AddDispatchItem(const OrderListItem& item)
{
    m_items.Add(const_cast<OrderListItem&>(item));
    if (m_tabList.GetCurSel() == 0) PopulateList();
}

// Recv 400: {"status":2000,"orders":[{order_id,store_name,pickup_addr,dest_addr,delivery_fee,elapsed_sec},...]}
// Recv 405: {"status":2000,"records":[{order_id,order_code,store_name,delivery_fee,status,created_at},...]}
LRESULT DeliveryListDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    auto toCS = [](const std::string& s) -> CString {
        CA2T ws(s.c_str(), CP_UTF8); return CString(ws);
    };

    try {
        json res = json::parse(body);
        if (res.value("status", 0) != STATUS_SUCCESS) return 0;

        if (protocol == CMD_RIDER_ORDER_LIST) {
            m_items.RemoveAll();
            for (const auto& o : res["orders"]) {
                OrderListItem item;
                item.orderId     = o.value("order_id",     0);
                item.storeName   = toCS(o.value("store_name",  ""));
                item.pickupAddr  = toCS(o.value("pickup_addr", ""));
                item.destAddr    = toCS(o.value("dest_addr",   ""));
                item.deliveryFee = o.value("delivery_fee",  0);
                item.elapsedSec  = o.value("elapsed_sec",   0);
                m_items.Add(item);
            }
            PopulateList();
        } else if (protocol == CMD_RIDER_MY_LIST) {
            m_listOrders.DeleteAllItems();
            int row = 0;
            for (const auto& r : res["records"]) {
                CString fee; fee.Format(_T("%d원"), r.value("delivery_fee", 0));
                int nRow = m_listOrders.InsertItem(row++, toCS(r.value("created_at", "")));
                m_listOrders.SetItemText(nRow, 1, toCS(r.value("order_code",  "")));
                m_listOrders.SetItemText(nRow, 2, toCS(r.value("store_name",  "")));
                m_listOrders.SetItemText(nRow, 3, fee);
                m_listOrders.SetItemText(nRow, 4, toCS(r.value("status", "")));
            }
        }
    } catch (...) {}
    return 0;
}

HBRUSH DeliveryListDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg)
            m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    // Buttons: do NOT override - let Windows draw button text normally
    return hbr;
}
