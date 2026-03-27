// ============================================================
// MainDlg.cpp
// :
// - OnSocketRecv: RecvPacket JSON
// - OnDispatchPush: 408 Push JSON
// - SendStatusToServer: JSON
// - OnBtnStartDrive: JSON { "action":"ONLINE" }
// ============================================================
#include "pch.h"
#include "MainDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "DispatchDlg.h"
#include "MyPageDlg.h"
#include "DeliveryListDlg.h"
#include "ChatDlg.h"
#include "PickupCodeDlg.h"
#include "DeliveryPhotoDlg.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(MainDlg, CDialogEx)

BEGIN_MESSAGE_MAP(MainDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_START_DRIVE,    &MainDlg::OnBtnStartDrive)
    ON_BN_CLICKED(IDC_BTN_MY_PAGE,        &MainDlg::OnBtnMyPage)
    ON_BN_CLICKED(IDC_BTN_DELIVERY_LIST,  &MainDlg::OnBtnDeliveryList)
    ON_BN_CLICKED(IDC_BTN_STEP_ACTION,    &MainDlg::OnBtnStepAction)
    ON_BN_CLICKED(IDC_CHECK_NEW_DISPATCH, &MainDlg::OnCheckNewDispatch)
    ON_MESSAGE(WM_SOCKET_RECV,            &MainDlg::OnSocketRecv)
    ON_MESSAGE(WM_DISPATCH_PUSH,          &MainDlg::OnDispatchPush)
    ON_MESSAGE(WM_SERVER_DISCONN,         &MainDlg::OnServerDisconn)
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_HELP, &MainDlg::OnBtnHelp)
END_MESSAGE_MAP()

MainDlg::MainDlg(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DLG, pParent)
{
}

MainDlg::~MainDlg()
{
}

// ──────────────────────────────────────────────────────
// 강제 종료(X 버튼, Alt+F4) 시 서버에 로그아웃 패킷 전송
// ──────────────────────────────────────────────────────
void MainDlg::OnClose()
{
    // 1. 타이머 먼저 정리 (WM_TIMER가 소멸된 창에 전달되는 것 방지)
    KillTimer(1);
    KillTimer(2);

    // 2. 소켓 수신 라우팅 해제 (RecvThread PostMessage 방지)
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_STATUS_UPDATE);
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_ACCEPT);
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_PICKUP_DONE);
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_DELIVERY_DONE);
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_ORDER_LIST);
    AppContext::Get().socket.UnregisterWnd(CMD_RIDER_MY_LIST);
    AppContext::Get().socket.SetNotifyWnd(nullptr);

    // 3. 서버에 로그아웃 전송 후 연결 해제
    if (AppContext::Get().session.isLoggedIn) {
        AppContext::Get().socket.SendPacket(CMD_LOGOUT, "{}");
        AppContext::Get().session.isLoggedIn = false;
    }
    AppContext::Get().socket.Disconnect();

    CDialogEx::OnClose();
}

// 도움요청 버튼 → 관리자 채팅창 열기
void MainDlg::OnBtnHelp()
{
    ChatDlg dlg(this);
    dlg.DoModal();
    // 채팅 종료 후 이전 WM_SOCKET_RECV 핸들러 복원
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_STATUS_UPDATE, GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_ACCEPT,        GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_PICKUP_DONE,   GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_DELIVERY_DONE, GetSafeHwnd());
}

void MainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_MAP,         m_staticMap);
    DDX_Control(pDX, IDC_CHECK_NEW_DISPATCH, m_checkNewDispatch);
    DDX_Control(pDX, IDC_BTN_START_DRIVE,    m_btnStartDrive);
    DDX_Control(pDX, IDC_BTN_STEP_ACTION,    m_btnStepAction);
    DDX_Control(pDX, IDC_BTN_MY_PAGE,        m_btnMyPage);
    DDX_Control(pDX, IDC_BTN_DELIVERY_LIST,  m_btnDeliveryList);
}

BOOL MainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_STATUS_UPDATE, GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_ACCEPT,        GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_PICKUP_DONE,   GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_DELIVERY_DONE, GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_ORDER_LIST,    GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_MY_LIST,       GetSafeHwnd());
    m_checkNewDispatch.SetCheck(BST_CHECKED);
    SetTimer(1, 1000, nullptr);
    SetStep(DeliveryStep::IDLE);

    CString title;
    title.Format(_T("      - %s    "),
                 AppContext::Get().session.name.IsEmpty()
                     ? _T("운천역")
                     : static_cast<LPCTSTR>(AppContext::Get().session.name));
    SetWindowText(title);
    return TRUE;
}

// 
// /
// JSON { "action": "ONLINE" / "OFFLINE" }
// 
void MainDlg::OnBtnStartDrive()
{
    if (!m_bDriving) {
        m_bDriving    = true;
        m_dwStartTime = GetTickCount64();
        AppContext::Get().session.isOnline        = true;
        AppContext::Get().session.drivingStartTick = m_dwStartTime;
        SetDlgItemText(IDC_BTN_START_DRIVE, _T("운행 종료"));
        SetStep(DeliveryStep::ONLINE);

        json req; req["action"] = "ONLINE";
        AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, req.dump());
    } else {
        if (m_step != DeliveryStep::ONLINE && m_step != DeliveryStep::IDLE) {
            MessageBox(_T("배달 진행 중에는 운행을 종료할 수 없습니다."),
                       _T("알림"), MB_OK | MB_ICONWARNING);
            return;
        }
        if (MessageBox(_T("운행을 종료하시겠습니까?"),
                       _T("운행 종료"), MB_YESNO | MB_ICONQUESTION) != IDYES)
            return;

        m_bDriving = false;
        AppContext::Get().session.isOnline = false;
        // 이번 운행 시간을 누적
        if (AppContext::Get().session.drivingStartTick > 0) {
            ULONGLONG sec = (GetTickCount64() - AppContext::Get().session.drivingStartTick) / 1000;
            AppContext::Get().session.totalDriveSec += sec;
            AppContext::Get().session.drivingStartTick = 0;
        }
        SetDlgItemText(IDC_BTN_START_DRIVE, _T("운행 시작"));
        SetStep(DeliveryStep::IDLE);

        json req; req["action"] = "OFFLINE";
        AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, req.dump());
        SetDlgItemText(IDC_STATIC_STATUS, _T("Drive stopped."));
    }
}

void MainDlg::OnBtnMyPage()
{
    MyPageDlg dlg(this);
    dlg.DoModal();
    // Restore routing back to MainDlg after sub-dialog closes
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_MY_LIST, GetSafeHwnd());
    if (!AppContext::Get().session.isLoggedIn)
        EndDialog(IDCANCEL);
}

void MainDlg::OnBtnDeliveryList()
{
    DeliveryListDlg dlg(this);
    dlg.DoModal();
    // Restore routing back to MainDlg after sub-dialog closes
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_ORDER_LIST, GetSafeHwnd());
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_MY_LIST,    GetSafeHwnd());
    if (AppContext::Get().currentOrder.IsActive() &&
        m_step == DeliveryStep::ONLINE)
    {
        SetStep(DeliveryStep::PICKUP_MOVING);
    }
}

void MainDlg::OnBtnStepAction()
{
    switch (m_step) {
    case DeliveryStep::PICKUP_MOVING:
        SetStep(DeliveryStep::STORE_ARRIVED);
        break;

    case DeliveryStep::STORE_ARRIVED: {
        PickupCodeDlg dlg(this);
        if (dlg.DoModal() == IDOK)
            SetStep(DeliveryStep::PICKED_UP);
        break;
    }

    case DeliveryStep::PICKED_UP:
        SetStep(DeliveryStep::DELIVERING);
        break;

    case DeliveryStep::DELIVERING:
        SetStep(DeliveryStep::DEST_ARRIVED);
        break;

    case DeliveryStep::DEST_ARRIVED: {
        DeliveryPhotoDlg dlg(this);
        if (dlg.DoModal() == IDOK) {
            SetStep(DeliveryStep::DELIVERED);
            SetTimer(2, 2000, nullptr);
        }
        break;
    }

    default: break;
    }
}

void MainDlg::OnCheckNewDispatch()
{
    bool bOn = (m_checkNewDispatch.GetCheck() == BST_CHECKED);
    json req;
    req["action"] = bOn ? "DISPATCH_ON" : "DISPATCH_OFF";
    AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, req.dump());
}

// 
//
// 
void MainDlg::SetStep(DeliveryStep s)
{
    m_step = s;
    SendStatusToServer(StepToString(s));
    UpdateStepUI();
}

CString MainDlg::StepToString(DeliveryStep s)
{
    switch (s) {
    case DeliveryStep::IDLE:          return _T("IDLE");
    case DeliveryStep::ONLINE:        return _T("ONLINE");
    case DeliveryStep::PICKUP_MOVING: return _T("PICKUP_MOVING");
    case DeliveryStep::STORE_ARRIVED: return _T("STORE_ARRIVED");
    case DeliveryStep::PICKED_UP:     return _T("PICKED_UP");
    case DeliveryStep::DELIVERING:    return _T("DELIVERING");
    case DeliveryStep::DEST_ARRIVED:  return _T("DEST_ARRIVED");
    case DeliveryStep::DELIVERED:     return _T("DELIVERED");
    default:                          return _T("UNKNOWN");
    }
}

// (JSON )
void MainDlg::SendStatusToServer(const CString& statusStr)
{
    int orderId = AppContext::Get().currentOrder.orderId;

    // IDLE / ONLINE CMD
    if (statusStr == _T("ONLINE") || statusStr == _T("OFFLINE")
        || statusStr == _T("IDLE")) return; // OnBtnStartDrive 

    //
    if (statusStr == _T("PICKED_UP") && orderId > 0) {
        json req; req["order_id"] = orderId;
        AppContext::Get().socket.SendPacket(CMD_RIDER_PICKUP_DONE, req.dump());
        return;
    }

    // DeliveryPhotoDlg OK
    // (SetStep(DELIVERED) OnBtnStepAction DeliveryPhotoDlg::DoModal)
}

// 
// UI
// 
void MainDlg::UpdateStepUI()
{
    CString statusText, stepInfo, btnText;
    bool    bBtnEnabled = m_bDriving;
    const CurrentOrder& ord = AppContext::Get().currentOrder;

    switch (m_step) {
    case DeliveryStep::IDLE:
        statusText = _T("운행 대기 중");
        stepInfo   = _T("운행 시작 버튼을 눌러 배달을 시작하세요.");
        btnText    = _T(""); bBtnEnabled = false; break;

    case DeliveryStep::ONLINE:
        statusText  = _T("배차 대기 중");
        stepInfo    = _T("신규 배달 요청을 기다리고 있습니다.");
        btnText     = _T(""); bBtnEnabled = false; break;

    case DeliveryStep::PICKUP_MOVING:
        statusText = _T("픽업 이동 중");
        stepInfo.Format(_T("픽업지: %s"), static_cast<LPCTSTR>(ord.pickupAddress));
        btnText    = _T("가게 도착"); break;

    case DeliveryStep::STORE_ARRIVED:
        statusText = _T("가게 도착");
        stepInfo.Format(_T("주문 픽업을 완료해주세요.\n주문코드: %s"),
                        static_cast<LPCTSTR>(ord.orderCode));
        btnText    = _T("주문번호 픽업"); break;

    case DeliveryStep::PICKED_UP:
        statusText = _T("픽업 완료");
        stepInfo.Format(_T("전달지로 이동하세요.\n전달지: %s"),
                        static_cast<LPCTSTR>(ord.deliveryAddress));
        btnText    = _T("전달지 이동"); break;

    case DeliveryStep::DELIVERING:
        statusText = _T("배달 중");
        stepInfo.Format(_T("전달지: %s"), static_cast<LPCTSTR>(ord.deliveryAddress));
        btnText    = _T("전달지 도착"); break;

    case DeliveryStep::DEST_ARRIVED:
        statusText = _T("전달지 도착");
        stepInfo.Format(_T("고객 요청사항: %s"),
                        static_cast<LPCTSTR>(ord.customerRequest));
        btnText    = _T("전달 완료"); break;

    case DeliveryStep::DELIVERED:
        statusText = _T("배달 완료!");
        stepInfo.Format(_T("배달료 %d원 정산 예정.\n다음 배달을 기다립니다."),
                        ord.deliveryFee);
        btnText     = _T(""); bBtnEnabled = false; break;
    }

    SetDlgItemText(IDC_STATIC_STATUS,    statusText);
    SetDlgItemText(IDC_STATIC_STEP_INFO, stepInfo);

    CWnd* pBtn = GetDlgItem(IDC_BTN_STEP_ACTION);
    if (pBtn) {
        pBtn->SetWindowText(btnText);
        pBtn->EnableWindow(bBtnEnabled && !btnText.IsEmpty());
        pBtn->ShowWindow(btnText.IsEmpty() ? SW_HIDE : SW_SHOW);
    }
    m_staticMap.Invalidate();
    Invalidate(FALSE);
}

// 
//
// 
void MainDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        if (m_bDriving && m_dwStartTime > 0) {
            ULONGLONG elapsed = (GetTickCount64() - m_dwStartTime) / 1000;
            ULONGLONG h = elapsed / 3600, m = (elapsed % 3600) / 60, s = elapsed % 60;
            CString timeStr, title;
            timeStr.Format(_T("    : %02u:%02u:%02u"), h, m, s);
            title.Format(_T("      (%s) - %s"), static_cast<LPCTSTR>(timeStr),
                         static_cast<LPCTSTR>(AppContext::Get().session.name));
            SetWindowText(title);
        }
    } else if (nIDEvent == 2) {
        KillTimer(2);
        AppContext::Get().currentOrder.Clear();
        SetStep(DeliveryStep::ONLINE);
    }
    CDialogEx::OnTimer(nIDEvent);
}

// 
//
// 
void MainDlg::OnPaint()
{
    CDialogEx::OnPaint();
    CRect mapRect;
    m_staticMap.GetWindowRect(&mapRect);
    ScreenToClient(&mapRect);
    CDC* pDC = GetDC();
    if (!pDC) return;
    DrawSimpleMap(pDC, mapRect);
    ReleaseDC(pDC);
}

void MainDlg::DrawSimpleMap(CDC* pDC, const CRect& rect)
{
    CBrush bgBrush(RGB(220, 240, 220));
    pDC->FillRect(&rect, &bgBrush);

    CPen gridPen(PS_SOLID, 1, RGB(180, 210, 180));
    CPen* pOldPen = pDC->SelectObject(&gridPen);
    const int GRID = 30;
    for (int x = rect.left; x < rect.right; x += GRID) { pDC->MoveTo(x, rect.top); pDC->LineTo(x, rect.bottom); }
    for (int y = rect.top;  y < rect.bottom; y += GRID) { pDC->MoveTo(rect.left, y); pDC->LineTo(rect.right, y); }

    CPen roadPen(PS_SOLID, 3, RGB(200, 200, 190));
    pDC->SelectObject(&roadPen);
    int midX = rect.left + rect.Width() / 2;
    int midY = rect.top  + rect.Height() / 2;
    pDC->MoveTo(rect.left, midY); pDC->LineTo(rect.right, midY);
    pDC->MoveTo(midX, rect.top);  pDC->LineTo(midX, rect.bottom);

    const int R = 10;
    CPen riderPen(PS_SOLID, 2, RGB(0, 120, 80));
    CBrush riderBrush(RGB(30, 160, 117));
    pDC->SelectObject(&riderPen); pDC->SelectObject(&riderBrush);
    pDC->Ellipse(midX - R, midY - R, midX + R, midY + R);

    if (m_step >= DeliveryStep::PICKUP_MOVING && m_step <= DeliveryStep::STORE_ARRIVED) {
        int pX = midX + 60, pY = midY - 40;
        CPen pickPen(PS_SOLID, 2, RGB(180, 0, 0)); CBrush pickBrush(RGB(220, 50, 50));
        pDC->SelectObject(&pickPen); pDC->SelectObject(&pickBrush);
        pDC->Ellipse(pX-R, pY-R, pX+R, pY+R);
        pDC->SetTextColor(RGB(180,0,0)); pDC->SetBkMode(TRANSPARENT);
        pDC->TextOut(pX-8, pY+R+2, _T("픽업"));
    }
    if (m_step >= DeliveryStep::PICKED_UP) {
        int dX = midX - 50, dY = midY - 60;
        CPen destPen(PS_SOLID, 2, RGB(0, 50, 180)); CBrush destBrush(RGB(50, 100, 220));
        pDC->SelectObject(&destPen); pDC->SelectObject(&destBrush);
        pDC->Ellipse(dX-R, dY-R, dX+R, dY+R);
        pDC->SetTextColor(RGB(0,50,180)); pDC->SetBkMode(TRANSPARENT);
        pDC->TextOut(dX-8, dY+R+2, _T("전달"));
    }

    pDC->SetTextColor(RGB(80,80,80)); pDC->SetBkMode(TRANSPARENT);
    pDC->TextOut(rect.left+5, rect.top+5,         _T("상무지구"));
    pDC->TextOut(rect.right-60, rect.top+5,       _T("운천역"));
    pDC->TextOut(rect.left+5, rect.bottom-20,     _T("서구청"));
    pDC->TextOut(rect.right-90, rect.bottom-20,   _T("광주WC경기장"));
    pDC->SelectObject(pOldPen);
}

// 
// (RecvPacket JSON )
// 
LRESULT MainDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    uint16_t protocol = pPkt->protocol;
    std::string body  = pPkt->body;
    delete pPkt;

    try {
        json res = json::parse(body);
        int status = res.value("status", 0);

        switch (protocol) {
        case CMD_RIDER_ACCEPT:         // 401: 
            if (status == STATUS_SUCCESS) {
                // currentOrder DispatchDlg
                //
                if (res.contains("order_code")) {
                    CA2T cs(res["order_code"].get<std::string>().c_str(), CP_UTF8);
                    AppContext::Get().currentOrder.orderCode = CString(cs);
                }
                SetStep(DeliveryStep::PICKUP_MOVING);
            }
            break;

        case CMD_RIDER_PICKUP_DONE:    // 403: 
            // UI OnBtnStepAction
            //
            break;

        case CMD_RIDER_DELIVERY_DONE:  // 404: 
            if (status == STATUS_SUCCESS) {
                int fee = res.value("delivery_fee", 0);
                std::string msg = res.value("message", "           !");
                CA2T wMsg(msg.c_str(), CP_UTF8);
                MessageBox(CString(wMsg), _T("운행 종료"), MB_OK | MB_ICONINFORMATION);
                AppContext::Get().currentOrder.deliveryFee = fee;
            }
            break;

        case CMD_RIDER_STATUS_UPDATE:  // 406: 
            // UI ( SetStep )
            break;

        default:
            break;
        }
    } catch (...) {}

    return 0;
}

// 
// Push: (408 NTF_NEW_DISPATCH)
// Push JSON:
// { "order_id":1, "store_name":"...",
// "pickup_addr":"...", "dest_addr":"...", "delivery_fee":3000 }
// 
LRESULT MainDlg::OnDispatchPush(WPARAM /*w*/, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    std::string body = pPkt->body;
    delete pPkt;

    if (!m_bDriving) return 0;
    if (m_checkNewDispatch.GetCheck() != BST_CHECKED) return 0;
    if (m_step != DeliveryStep::ONLINE) return 0;

    try {
        json push = json::parse(body);

        // DispatchDlg pushData CString
        // "700|orderId|storeName|pickupAddr|destAddr|deliveryFee"
        // DispatchDlg ParsePushData()
        auto toCS = [](const std::string& s) -> CString {
            CA2T ws(s.c_str(), CP_UTF8);
            return CString(ws);
        };

        int orderId      = push.value("order_id",     0);
        CString store    = toCS(push.value("store_name",  ""));
        CString pickup   = toCS(push.value("pickup_addr", ""));
        CString dest     = toCS(push.value("dest_addr",   ""));
        int fee          = push.value("delivery_fee",  0);

        CString pushData;
        pushData.Format(_T("700|%d|%s|%s|%s|%d"),
                        orderId, static_cast<LPCTSTR>(store),
                        static_cast<LPCTSTR>(pickup),
                        static_cast<LPCTSTR>(dest), fee);

        DispatchDlg dlg(pushData, this);
        if (dlg.DoModal() == IDOK)
            SetStep(DeliveryStep::PICKUP_MOVING);

    } catch (...) {}

    return 0;
}

LRESULT MainDlg::OnServerDisconn(WPARAM /*w*/, LPARAM /*l*/)
{
    if (IsWindowVisible())
        MessageBox(_T("Server connection lost.\nPlease check your network."),
                   _T("Connection Error"), MB_OK | MB_ICONWARNING);
    return 0;
}

HBRUSH MainDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
