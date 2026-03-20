#include "pch.h"
#include "DeliveryListDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "DispatchDlg.h"

IMPLEMENT_DYNAMIC(DeliveryListDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DeliveryListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REFRESH,      &DeliveryListDlg::OnBtnRefresh)
    ON_BN_CLICKED(IDC_BTN_ACCEPT_ITEM,  &DeliveryListDlg::OnBtnAcceptItem)
    ON_BN_CLICKED(IDC_BTN_REJECT_ITEM,  &DeliveryListDlg::OnBtnRejectItem)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_ORDERS, &DeliveryListDlg::OnDblclkList)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_LIST, &DeliveryListDlg::OnTabSelChange)
    ON_MESSAGE(WM_SOCKET_RECV,          &DeliveryListDlg::OnSocketRecv)
    ON_WM_TIMER()
END_MESSAGE_MAP()

DeliveryListDlg::DeliveryListDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_LIST_DLG, pParent)
{
}

DeliveryListDlg::~DeliveryListDlg()
{
}

void DeliveryListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_ORDERS, m_listOrders);
    DDX_Control(pDX, IDC_TAB_LIST,    m_tabList);
}

BOOL DeliveryListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 탭 추가
    TC_ITEM ti = {};
    ti.mask = TCIF_TEXT;
    ti.pszText = const_cast<LPTSTR>(_T("Current"));
    m_tabList.InsertItem(0, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("History"));
    m_tabList.InsertItem(1, &ti);

    // 소켓 알림 윈도우
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    InitListCtrl();
    RefreshCurrentList();

    // 경과시간 1초 갱신 타이머
    SetTimer(1, 1000, nullptr);

    return TRUE;
}

// ─────────────────────────────────────────────
// CListCtrl 컬럼 초기화
// ─────────────────────────────────────────────
void DeliveryListDlg::InitListCtrl()
{
    m_listOrders.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listOrders.DeleteAllItems();

    // 기존 컬럼 삭제
    while (m_listOrders.DeleteColumn(0)) {}

    if (m_tabList.GetCurSel() == 0) {
        // 현재 요청 컬럼
        m_listOrders.InsertColumn(0, _T("Elapsed"),  LVCFMT_LEFT, 80);
        m_listOrders.InsertColumn(1, _T("Store"),    LVCFMT_LEFT, 120);
        m_listOrders.InsertColumn(2, _T("Pickup"),    LVCFMT_LEFT, 150);
        m_listOrders.InsertColumn(3, _T("Dest"),    LVCFMT_LEFT, 150);
        m_listOrders.InsertColumn(4, _T("Fee"),    LVCFMT_RIGHT, 70);
    } else {
        // 이전 내역 컬럼
        m_listOrders.InsertColumn(0, _T("Date/Time"), LVCFMT_LEFT, 130);
        m_listOrders.InsertColumn(1, _T("OrderCode"),  LVCFMT_LEFT, 90);
        m_listOrders.InsertColumn(2, _T("Store"),    LVCFMT_LEFT, 120);
        m_listOrders.InsertColumn(3, _T("Fee"),    LVCFMT_RIGHT, 70);
        m_listOrders.InsertColumn(4, _T("Status"),      LVCFMT_LEFT, 70);
    }
}

// ─────────────────────────────────────────────
// 현재 요청 목록 서버 요청
// ─────────────────────────────────────────────
void DeliveryListDlg::RefreshCurrentList()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_ORDER_LIST);
    if (!bSent) {
        // 서버 미연결 시 더미 데이터
        m_items.RemoveAll();

        OrderListItem item1;
        item1.orderId     = 1001;
        item1.storeName   = _T("Chicken Sangmu");
        item1.pickupAddr  = _T("Sangmu-daero 123");
        item1.destAddr    = _T("Chipyeong 456");
        item1.deliveryFee = 3500;
        item1.elapsedSec  = 130;
        m_items.Add(item1);

        OrderListItem item2;
        item2.orderId     = 1002;
        item2.storeName   = _T("PizzaHut Sangmu");
        item2.pickupAddr  = _T("Sangmu-jungang 45");
        item2.destAddr    = _T("Mareuk 789");
        item2.deliveryFee = 4000;
        item2.elapsedSec  = 330;
        m_items.Add(item2);

        PopulateList();
    }
}

// ─────────────────────────────────────────────
// 현재 요청 목록 → CListCtrl 채우기
// ─────────────────────────────────────────────
void DeliveryListDlg::PopulateList()
{
    m_listOrders.DeleteAllItems();

    for (INT_PTR i = 0; i < m_items.GetSize(); i++) {
        const OrderListItem& item = m_items[i];

        // 경과시간 포맷: mm분 ss초
        int m = item.elapsedSec / 60;
        int s = item.elapsedSec % 60;
        CString elapsed;
        elapsed.Format(_T("%dm %02ds"), m, s);

        CString fee;
        fee.Format(_T("%dW"), item.deliveryFee);

        int nRow = m_listOrders.InsertItem((int)i, elapsed);
        m_listOrders.SetItemText(nRow, 1, item.storeName);
        m_listOrders.SetItemText(nRow, 2, item.pickupAddr);
        m_listOrders.SetItemText(nRow, 3, item.destAddr);
        m_listOrders.SetItemText(nRow, 4, fee);
        m_listOrders.SetItemData(nRow, (DWORD_PTR)item.orderId);
    }
}

// ─────────────────────────────────────────────
// 이전 내역 목록 서버 요청
// ─────────────────────────────────────────────
void DeliveryListDlg::PopulateHistoryList()
{
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_MY_LIST);
    if (!bSent) {
        // 서버 미연결 시 더미 데이터
        m_listOrders.DeleteAllItems();
        struct { LPCTSTR dt; LPCTSTR code; LPCTSTR store; int fee; } dummy[] = {
            { _T("03/20 19:35"), _T("2783JBCD"), _T("Chicken Sangmu"), 3500 },
            { _T("03/20 17:55"), _T("7824SJFE"), _T("PizzaHut Sangmu"),   4000 },
            { _T("03/20 15:10"), _T("0128VPLW"), _T("BurgerKing Sangmu"), 3000 },
        };
        for (int i = 0; i < 3; i++) {
            CString fee;
            fee.Format(_T("%dW"), dummy[i].fee);
            int nRow = m_listOrders.InsertItem(i, dummy[i].dt);
            m_listOrders.SetItemText(nRow, 1, dummy[i].code);
            m_listOrders.SetItemText(nRow, 2, dummy[i].store);
            m_listOrders.SetItemText(nRow, 3, fee);
            m_listOrders.SetItemText(nRow, 4, _T("Done"));
        }
    }
}

// ─────────────────────────────────────────────
// 경과시간 1초 갱신
// ─────────────────────────────────────────────
void DeliveryListDlg::UpdateElapsedTime()
{
    if (m_tabList.GetCurSel() != 0) return;

    for (INT_PTR i = 0; i < m_items.GetSize(); i++) {
        m_items[i].elapsedSec++;
        int m = m_items[i].elapsedSec / 60;
        int s = m_items[i].elapsedSec % 60;
        CString elapsed;
        elapsed.Format(_T("%dm %02ds"), m, s);
        m_listOrders.SetItemText((int)i, 0, elapsed);
    }
}

// ─────────────────────────────────────────────
// 새로고침 버튼
// ─────────────────────────────────────────────
void DeliveryListDlg::OnBtnRefresh()
{
    if (m_tabList.GetCurSel() == 0) {
        m_items.RemoveAll();
        RefreshCurrentList();
    } else {
        PopulateHistoryList();
    }
}

// ─────────────────────────────────────────────
// 수락 버튼 (선택 항목 → DispatchDlg)
// ─────────────────────────────────────────────
void DeliveryListDlg::OnBtnAcceptItem()
{
    int nSel = m_listOrders.GetNextItem(-1, LVNI_SELECTED);
    if (nSel < 0) {
        MessageBox(_T("Select an item to accept."), _T("Notice"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (nSel >= (int)m_items.GetSize()) return;

    const OrderListItem& item = m_items[nSel];

    // DispatchDlg용 pushData 생성
    CString pushData;
    pushData.Format(_T("700|%d|%s|%s|%s|%d"),
                    item.orderId,
                    static_cast<LPCTSTR>(item.storeName),
                    static_cast<LPCTSTR>(item.pickupAddr),
                    static_cast<LPCTSTR>(item.destAddr),
                    item.deliveryFee);

    DispatchDlg dlg(pushData, this);
    if (dlg.DoModal() == IDOK) {
        // 수락 완료 → 목록에서 제거
        m_items.RemoveAt(nSel);
        PopulateList();
        EndDialog(IDOK);  // MainDlg에 배차 수락 알림
    }
}

// ─────────────────────────────────────────────
// 거절 버튼
// ─────────────────────────────────────────────
void DeliveryListDlg::OnBtnRejectItem()
{
    int nSel = m_listOrders.GetNextItem(-1, LVNI_SELECTED);
    if (nSel < 0 || nSel >= (int)m_items.GetSize()) return;

    CString payload;
    payload.Format(_T("%d|MANUAL"), m_items[nSel].orderId);
    AppContext::Get().socket.SendPacket(CMD_RIDER_REJECT, payload);

    m_items.RemoveAt(nSel);
    PopulateList();
}

// ─────────────────────────────────────────────
// 더블클릭 → 수락 처리
// ─────────────────────────────────────────────
void DeliveryListDlg::OnDblclkList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    if (m_tabList.GetCurSel() == 0)
        OnBtnAcceptItem();
    *pResult = 0;
}

// ─────────────────────────────────────────────
// 탭 전환
// ─────────────────────────────────────────────
void DeliveryListDlg::OnTabSelChange(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    InitListCtrl();
    if (m_tabList.GetCurSel() == 0)
        RefreshCurrentList();
    else
        PopulateHistoryList();
    *pResult = 0;
}

// ─────────────────────────────────────────────
// 타이머
// ─────────────────────────────────────────────
void DeliveryListDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) UpdateElapsedTime();
    CDialogEx::OnTimer(nIDEvent);
}

// ─────────────────────────────────────────────
// 외부에서 배차 항목 추가 (MainDlg의 OnDispatchPush에서 호출)
// ─────────────────────────────────────────────
void DeliveryListDlg::AddDispatchItem(const OrderListItem& item)
{
    m_items.Add(const_cast<OrderListItem&>(item));
    if (m_tabList.GetCurSel() == 0)
        PopulateList();
}

// ─────────────────────────────────────────────
// 서버 수신 처리
// "400|OK|orderId,storeName,pickup,dest,fee;..." 형식
// ─────────────────────────────────────────────
LRESULT DeliveryListDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int pipePos = msg.Find(_T('|'));
    if (pipePos < 0) return 0;
    int cmd = _ttoi(msg.Left(pipePos));

    if (cmd == CMD_RIDER_ORDER_LIST) {
        CString payload = msg.Mid(pipePos + 1);
        int ok = payload.Find(_T('|'));
        if (ok >= 0 && payload.Left(ok) == _T("OK"))
            ParseOrderListResponse(payload.Mid(ok + 1));
    } else if (cmd == CMD_RIDER_MY_LIST) {
        // 이전 내역 파싱은 PopulateHistoryList 서버 응답 버전에서 처리
    }
    return 0;
}

// ─────────────────────────────────────────────
// 서버 응답 파싱
// 항목 구분: ';'  /  필드 구분: ','
// "orderId,storeName,pickup,dest,fee"
// ─────────────────────────────────────────────
void DeliveryListDlg::ParseOrderListResponse(const CString& payload)
{
    m_items.RemoveAll();

    CString data = payload;
    while (!data.IsEmpty()) {
        int semi = data.Find(_T(';'));
        CString row = (semi >= 0) ? data.Left(semi) : data;
        data = (semi >= 0) ? data.Mid(semi + 1) : _T("");

        if (row.IsEmpty()) continue;

        OrderListItem item;
        auto nextField = [&](CString& out) {
            int comma = row.Find(_T(','));
            if (comma >= 0) {
                out = row.Left(comma);
                row = row.Mid(comma + 1);
            } else {
                out = row; row = _T("");
            }
        };

        CString tmp;
        nextField(tmp);  item.orderId     = _ttoi(tmp);
        nextField(tmp);  item.storeName   = tmp;
        nextField(tmp);  item.pickupAddr  = tmp;
        nextField(tmp);  item.destAddr    = tmp;
        nextField(tmp);  item.deliveryFee = _ttoi(tmp);
        item.elapsedSec = 0;

        m_items.Add(item);
    }

    PopulateList();
}
