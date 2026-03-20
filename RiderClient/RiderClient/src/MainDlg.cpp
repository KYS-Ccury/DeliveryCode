#include "pch.h"
#include "MainDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "DispatchDlg.h"
#include "MyPageDlg.h"
#include "DeliveryListDlg.h"
#include "PickupCodeDlg.h"
#include "DeliveryPhotoDlg.h"

IMPLEMENT_DYNAMIC(MainDlg, CDialogEx)

BEGIN_MESSAGE_MAP(MainDlg, CDialogEx)
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
END_MESSAGE_MAP()

MainDlg::MainDlg(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DLG, pParent)
{
}

MainDlg::~MainDlg()
{
}

void MainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_MAP,        m_staticMap);
    DDX_Control(pDX, IDC_CHECK_NEW_DISPATCH, m_checkNewDispatch);
}

BOOL MainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 소켓 알림 윈도우 설정
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // 신규배차 체크박스 기본 ON
    m_checkNewDispatch.SetCheck(BST_CHECKED);

    // 1초 타이머 시작 (운행시간 갱신용)
    SetTimer(1, 1000, nullptr);

    // 초기 UI 설정
    SetStep(DeliveryStep::IDLE);

    // 타이틀에 라이더 이름 표시
    CString title;
    title.Format(_T("배민커넥트 - %s 라이더"),
                 AppContext::Get().session.name.IsEmpty()
                     ? _T("라이더")
                     : static_cast<LPCTSTR>(AppContext::Get().session.name));
    SetWindowText(title);

    return TRUE;
}

// ─────────────────────────────────────────────
// 운행 시작/종료 토글
// ─────────────────────────────────────────────
void MainDlg::OnBtnStartDrive()
{
    if (!m_bDriving) {
        // 운행 시작
        m_bDriving   = true;
        m_dwStartTime = GetTickCount();
        AppContext::Get().session.isOnline = true;
        SetDlgItemText(IDC_BTN_START_DRIVE, _T("운행 종료"));
        SetStep(DeliveryStep::ONLINE);
        AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, _T("ONLINE"));
    } else {
        // 운행 종료 확인
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
        SetDlgItemText(IDC_BTN_START_DRIVE, _T("운행 시작"));
        SetStep(DeliveryStep::IDLE);
        AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, _T("OFFLINE"));
        SetDlgItemText(IDC_STATIC_STATUS, _T("운행 중지"));
    }
}

// ─────────────────────────────────────────────
// 마이페이지 버튼
// ─────────────────────────────────────────────
void MainDlg::OnBtnMyPage()
{
    MyPageDlg dlg(this);
    dlg.DoModal();
    // 마이페이지에서 로그아웃 시 처리
    if (!AppContext::Get().session.isLoggedIn) {
        EndDialog(IDCANCEL);
    }
}

// ─────────────────────────────────────────────
// 배달리스트 버튼
// ─────────────────────────────────────────────
void MainDlg::OnBtnDeliveryList()
{
    DeliveryListDlg dlg(this);
    dlg.DoModal();

    // DeliveryListDlg에서 배차 수락 시 (IDOK 반환 + currentOrder 채워짐)
    if (AppContext::Get().currentOrder.IsActive() &&
        m_step == DeliveryStep::ONLINE)
    {
        SetStep(DeliveryStep::PICKUP_MOVING);
    }
}

// ─────────────────────────────────────────────
// 단계별 액션 버튼
// ─────────────────────────────────────────────
void MainDlg::OnBtnStepAction()
{
    switch (m_step) {
    case DeliveryStep::PICKUP_MOVING:
        // 가게 도착
        SetStep(DeliveryStep::STORE_ARRIVED);
        break;

    case DeliveryStep::STORE_ARRIVED: {
        // 번호픽업 다이얼로그
        PickupCodeDlg dlg(this);
        if (dlg.DoModal() == IDOK)
            SetStep(DeliveryStep::PICKED_UP);
        break;
    }

    case DeliveryStep::PICKED_UP:
        // 전달지 이동 시작
        SetStep(DeliveryStep::DELIVERING);
        break;

    case DeliveryStep::DELIVERING:
        // 전달지 도착
        SetStep(DeliveryStep::DEST_ARRIVED);
        break;

    case DeliveryStep::DEST_ARRIVED: {
        // 전달 사진 촬영
        DeliveryPhotoDlg dlg(this);
        if (dlg.DoModal() == IDOK) {
            SetStep(DeliveryStep::DELIVERED);
            // 잠시 후 ONLINE 복귀
            SetTimer(2, 2000, nullptr);
        }
        break;
    }

    default:
        break;
    }
}

// ─────────────────────────────────────────────
// 신규배차 체크박스
// ─────────────────────────────────────────────
void MainDlg::OnCheckNewDispatch()
{
    bool bOn = (m_checkNewDispatch.GetCheck() == BST_CHECKED);
    AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE,
                                       bOn ? _T("DISPATCH_ON") : _T("DISPATCH_OFF"));
}

// ─────────────────────────────────────────────
// 단계 설정 + UI 갱신
// ─────────────────────────────────────────────
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

void MainDlg::SendStatusToServer(const CString& statusStr)
{
    CString payload;
    payload.Format(_T("%d|%s"), AppContext::Get().currentOrder.orderId, statusStr);
    AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE, payload);
}

// ─────────────────────────────────────────────
// 단계별 UI 갱신
// ─────────────────────────────────────────────
void MainDlg::UpdateStepUI()
{
    CString statusText, stepInfo, btnText;
    bool    bBtnEnabled = m_bDriving;

    const CurrentOrder& ord = AppContext::Get().currentOrder;

    switch (m_step) {
    case DeliveryStep::IDLE:
        statusText = _T("운행 대기 중");
        stepInfo   = _T("운행 시작 버튼을 눌러 배달을 시작하세요.");
        btnText    = _T("");
        bBtnEnabled = false;
        break;

    case DeliveryStep::ONLINE:
        statusText  = _T("배차 대기 중");
        stepInfo    = _T("신규 배달 요청을 기다리고 있습니다.");
        btnText     = _T("");
        bBtnEnabled = false;
        break;

    case DeliveryStep::PICKUP_MOVING:
        statusText = _T("픽업 이동 중");
        stepInfo.Format(_T("픽업지: %s"), static_cast<LPCTSTR>(ord.pickupAddress));
        btnText    = _T("가게 도착");
        break;

    case DeliveryStep::STORE_ARRIVED:
        statusText = _T("가게 도착");
        stepInfo.Format(_T("주문 픽업을 완료해주세요.\n주문코드: %s"),
                        static_cast<LPCTSTR>(ord.orderCode));
        btnText    = _T("주문번호 픽업");
        break;

    case DeliveryStep::PICKED_UP:
        statusText = _T("픽업 완료");
        stepInfo.Format(_T("전달지로 이동하세요.\n전달지: %s"),
                        static_cast<LPCTSTR>(ord.deliveryAddress));
        btnText    = _T("전달지 이동");
        break;

    case DeliveryStep::DELIVERING:
        statusText = _T("배달 중");
        stepInfo.Format(_T("전달지: %s"), static_cast<LPCTSTR>(ord.deliveryAddress));
        btnText    = _T("전달지 도착");
        break;

    case DeliveryStep::DEST_ARRIVED:
        statusText = _T("전달지 도착");
        stepInfo.Format(_T("고객 요청사항: %s"), static_cast<LPCTSTR>(ord.customerRequest));
        btnText    = _T("전달 완료");
        break;

    case DeliveryStep::DELIVERED:
        statusText = _T("배달 완료!");
        stepInfo.Format(_T("배달료 %d원 정산 예정.\n다음 배달을 기다립니다."),
                        ord.deliveryFee);
        btnText     = _T("");
        bBtnEnabled = false;
        break;
    }

    SetDlgItemText(IDC_STATIC_STATUS,    statusText);
    SetDlgItemText(IDC_STATIC_STEP_INFO, stepInfo);

    CWnd* pBtn = GetDlgItem(IDC_BTN_STEP_ACTION);
    if (pBtn) {
        pBtn->SetWindowText(btnText);
        pBtn->EnableWindow(bBtnEnabled && !btnText.IsEmpty());
        pBtn->ShowWindow(btnText.IsEmpty() ? SW_HIDE : SW_SHOW);
    }

    // 지도 영역 다시 그리기
    m_staticMap.Invalidate();
    Invalidate(FALSE);
}

// ─────────────────────────────────────────────
// 타이머
// ─────────────────────────────────────────────
void MainDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        // 운행시간 갱신
        if (m_bDriving && m_dwStartTime > 0) {
            DWORD elapsed = (GetTickCount() - m_dwStartTime) / 1000;
            DWORD h = elapsed / 3600;
            DWORD m = (elapsed % 3600) / 60;
            DWORD s = elapsed % 60;
            CString timeStr;
            timeStr.Format(_T("운행시간: %02u:%02u:%02u"), h, m, s);
            // IDC_STATIC_STATUS 하단에 별도 Static이 있으면 갱신
            // 현재는 타이틀에 반영
            CString title;
            title.Format(_T("배민커넥트 (%s) - %s"),
                         static_cast<LPCTSTR>(timeStr),
                         static_cast<LPCTSTR>(AppContext::Get().session.name));
            SetWindowText(title);
        }
    } else if (nIDEvent == 2) {
        // 배달 완료 후 ONLINE 복귀
        KillTimer(2);
        AppContext::Get().currentOrder.Clear();
        SetStep(DeliveryStep::ONLINE);
    }

    CDialogEx::OnTimer(nIDEvent);
}

// ─────────────────────────────────────────────
// 지도 그리기 (간이 격자 지도)
// ─────────────────────────────────────────────
void MainDlg::OnPaint()
{
    CDialogEx::OnPaint();

    // m_staticMap 영역에 격자 지도 그리기
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
    // 배경 (연한 녹색)
    CBrush bgBrush(RGB(220, 240, 220));
    pDC->FillRect(&rect, &bgBrush);

    // 격자선
    CPen gridPen(PS_SOLID, 1, RGB(180, 210, 180));
    CPen* pOldPen = pDC->SelectObject(&gridPen);

    const int GRID = 30;
    for (int x = rect.left; x < rect.right; x += GRID) {
        pDC->MoveTo(x, rect.top);
        pDC->LineTo(x, rect.bottom);
    }
    for (int y = rect.top; y < rect.bottom; y += GRID) {
        pDC->MoveTo(rect.left, y);
        pDC->LineTo(rect.right, y);
    }

    // 도로 (굵은 회색 선)
    CPen roadPen(PS_SOLID, 3, RGB(200, 200, 190));
    pDC->SelectObject(&roadPen);
    int midX = rect.left + rect.Width() / 2;
    int midY = rect.top  + rect.Height() / 2;
    pDC->MoveTo(rect.left, midY); pDC->LineTo(rect.right, midY);  // 가로 도로
    pDC->MoveTo(midX, rect.top);  pDC->LineTo(midX, rect.bottom); // 세로 도로

    // 라이더 위치 (녹색 원)
    int rX = midX, rY = midY;
    const int R = 10;
    CPen riderPen(PS_SOLID, 2, RGB(0, 120, 80));
    CBrush riderBrush(RGB(30, 160, 117));
    pDC->SelectObject(&riderPen);
    pDC->SelectObject(&riderBrush);
    pDC->Ellipse(rX - R, rY - R, rX + R, rY + R);

    // 픽업 마커 (빨간 원) - 배달 진행 중일 때만
    if (m_step >= DeliveryStep::PICKUP_MOVING && m_step <= DeliveryStep::STORE_ARRIVED) {
        int pX = midX + 60, pY = midY - 40;
        CPen pickPen(PS_SOLID, 2, RGB(180, 0, 0));
        CBrush pickBrush(RGB(220, 50, 50));
        pDC->SelectObject(&pickPen);
        pDC->SelectObject(&pickBrush);
        pDC->Ellipse(pX - R, pY - R, pX + R, pY + R);
        pDC->SetTextColor(RGB(180, 0, 0));
        pDC->SetBkMode(TRANSPARENT);
        pDC->TextOut(pX - 8, pY + R + 2, _T("픽업"));
    }

    // 전달지 마커 (파란 원) - 픽업 완료 후
    if (m_step >= DeliveryStep::PICKED_UP) {
        int dX = midX - 50, dY = midY - 60;
        CPen destPen(PS_SOLID, 2, RGB(0, 50, 180));
        CBrush destBrush(RGB(50, 100, 220));
        pDC->SelectObject(&destPen);
        pDC->SelectObject(&destBrush);
        pDC->Ellipse(dX - R, dY - R, dX + R, dY + R);
        pDC->SetTextColor(RGB(0, 50, 180));
        pDC->SetBkMode(TRANSPARENT);
        pDC->TextOut(dX - 8, dY + R + 2, _T("전달"));
    }

    // 지역명 텍스트
    pDC->SetTextColor(RGB(80, 80, 80));
    pDC->SetBkMode(TRANSPARENT);
    pDC->TextOut(rect.left + 5,  rect.top + 5,           _T("상무지구"));
    pDC->TextOut(rect.right - 60, rect.top + 5,          _T("운천역"));
    pDC->TextOut(rect.left + 5,  rect.bottom - 20,       _T("서구청"));
    pDC->TextOut(rect.right - 90, rect.bottom - 20,      _T("광주WC경기장"));

    pDC->SelectObject(pOldPen);
}

// ─────────────────────────────────────────────
// 서버 수신 처리
// CMD_RIDER_ACCEPT → 배차 완료
// CMD_RIDER_DELIVERY_DONE → 완료 확인
// ─────────────────────────────────────────────
LRESULT MainDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int pipePos = msg.Find(_T('|'));
    if (pipePos < 0) return 0;
    int cmd = _ttoi(msg.Left(pipePos));
    CString rest = msg.Mid(pipePos + 1);

    switch (cmd) {
    case CMD_RIDER_ACCEPT:
        if (rest.Left(2) == _T("OK")) {
            // 배차 확정 → 이미 DispatchDlg에서 currentOrder 채워져 있음
            SetStep(DeliveryStep::PICKUP_MOVING);
        }
        break;

    case CMD_RIDER_DELIVERY_DONE:
        if (rest.Left(2) == _T("OK")) {
            MessageBox(_T("배달이 완료되었습니다. 수고하셨습니다!"),
                       _T("배달 완료"), MB_OK | MB_ICONINFORMATION);
        }
        break;

    default:
        break;
    }
    return 0;
}

// ─────────────────────────────────────────────
// 서버 Push: 신규 배차 (PUSH_DISPATCH)
// ─────────────────────────────────────────────
LRESULT MainDlg::OnDispatchPush(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString pushData = *pMsg;
    delete pMsg;

    if (!m_bDriving) return 0;
    if (m_checkNewDispatch.GetCheck() != BST_CHECKED) return 0;
    if (m_step != DeliveryStep::ONLINE) return 0;  // 이미 배달 중이면 무시

    // 배차 팝업 표시
    DispatchDlg dlg(pushData, this);
    if (dlg.DoModal() == IDOK) {
        // 배차 수락 → 픽업 이동 단계로
        SetStep(DeliveryStep::PICKUP_MOVING);
    }
    return 0;
}

// ─────────────────────────────────────────────
// 서버 연결 끊김
// ─────────────────────────────────────────────
LRESULT MainDlg::OnServerDisconn(WPARAM /*w*/, LPARAM /*l*/)
{
    if (IsWindowVisible())
        MessageBox(_T("서버와의 연결이 끊어졌습니다.\n네트워크 상태를 확인해주세요."),
                   _T("연결 오류"), MB_OK | MB_ICONWARNING);
    return 0;
}
