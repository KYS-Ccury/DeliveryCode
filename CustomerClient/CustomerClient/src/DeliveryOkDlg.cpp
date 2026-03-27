// ================================================================
//  DeliveryOkDlg.cpp  (사장님 채팅 버튼 추가 버전)
//
//  기존 DeliveryOkDlg.cpp 에서 다음 3곳을 수정한다:
//
//  1. BEGIN_MESSAGE_MAP 에 ON_BN_CLICKED(IDC_BTN_CHAT) 추가
//  2. OnInitDialog 에서 채팅 버튼 표시
//  3. OnBnClickedBtnChat 함수 신규 추가
//
//  나머지 코드(RebuildInfoText, UpdateStatusUI 등)는 기존 그대로임.
//  전체 파일로 교체하거나 아래 '★ 변경' 표시 부분만 패치해도 된다.
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "DeliveryOkDlg.h"
#include "ChatDlg.h"          // ★ 추가
#include "OrderManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

IMPLEMENT_DYNAMIC(DeliveryOkDlg, CDialogEx)

DeliveryOkDlg::DeliveryOkDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_OK_DLG, pParent)
    , m_nTotalAmount(0)
    , m_nEstimatedMinutes(30)
    , m_nUsedPoint(0)
    , m_nDeliveryFee(0)
    , m_nOrderId(0)           // ★ 초기화
{}
DeliveryOkDlg::~DeliveryOkDlg() {}

void DeliveryOkDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

// ★ IDC_BTN_CHAT 핸들러 추가
BEGIN_MESSAGE_MAP(DeliveryOkDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                 &DeliveryOkDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BTN_CHAT,         &DeliveryOkDlg::OnBnClickedBtnChat) // ★ 추가
    ON_MESSAGE(WM_ORDER_STATUS_CHANGED, &DeliveryOkDlg::OnOrderStatusChanged)
END_MESSAGE_MAP()

// ================================================================
//  OnInitDialog
// ================================================================
BOOL DeliveryOkDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CString strTitle;
    strTitle.Format(_T("주문완료 - %s"), (LPCTSTR)m_strOrderID);
    SetWindowText(strTitle);

    SetDlgItemText(IDC_STATIC_STORE_NAME, m_strStoreName);
    SetDlgItemText(IDC_STATIC_ORDER_LIST, m_strOrderList);

    RebuildInfoText();

    // 리뷰 버튼 숨김 (배달 완료 전까지)
    CWnd* pReviewBtn = GetDlgItem(IDC_BTN_WRITE_REVIEW);
    if (pReviewBtn) pReviewBtn->ShowWindow(SW_HIDE);

    // ★ 채팅 버튼 표시 및 레이블 설정
    CWnd* pChatBtn = GetDlgItem(IDC_BTN_CHAT);
    if (pChatBtn) {
        pChatBtn->ShowWindow(SW_SHOW);
        pChatBtn->SetWindowText(_T("가게에 문의"));
    }

    // 주문 상태 변경 콜백 등록
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::NTF_ORDER_STATUS,
        [hThis](uint16_t, const std::string& body) {
            std::string token = "\"status\":";
            auto pos = body.find(token);
            int status = 0;
            if (pos != std::string::npos)
                try { status = std::stoi(body.substr(pos + token.size())); } catch (...) {}
            ::PostMessage(hThis, WM_ORDER_STATUS_CHANGED, (WPARAM)(new int(status)), 0);
        });

    OrderManager::GetInstance().RegisterOrderStatusCallback(
        [hThis](const std::string&, int status, const std::string&) {
            ::PostMessage(hThis, WM_ORDER_STATUS_CHANGED, (WPARAM)(new int(status)), 0);
        });

    UpdateStatusUI(STATUS_WAITING);
    return TRUE;
}

// ================================================================
//  OnBnClickedBtnChat  ★ 신규
//  사장님과의 1:1 채팅 다이얼로그를 열기
//
//  ▶ ChatDlg 에 전달하는 파라미터
//    m_strTargetName = 가게명  (타이틀 표시용)
//    m_strTargetType = "owner" (서버에 CUSTOMER_OWNER 방 요청)
//    m_nOrderId      = 현재 주문 ID (방 식별 키)
// ================================================================
void DeliveryOkDlg::OnBnClickedBtnChat()
{
    if (m_nOrderId <= 0) {
        AfxMessageBox(_T("주문 정보를 불러오는 중입니다. 잠시 후 다시 시도하세요."),
                      MB_ICONINFORMATION);
        return;
    }

    ChatDlg dlg(this);
    dlg.m_strTargetName = m_strStoreName.IsEmpty()
                          ? _T("가게 문의")
                          : m_strStoreName + _T(" 문의");
    dlg.m_strTargetType = _T("owner");
    dlg.m_nOrderId      = m_nOrderId;

    dlg.DoModal();
}

// ================================================================
//  OnBnClickedOk
// ================================================================
void DeliveryOkDlg::OnBnClickedOk()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    CDialogEx::OnOK();
}

// ================================================================
//  OnOrderStatusChanged
// ================================================================
LRESULT DeliveryOkDlg::OnOrderStatusChanged(WPARAM wParam, LPARAM)
{
    int* p = reinterpret_cast<int*>(wParam);
    int status = p ? *p : 0;
    delete p;
    UpdateStatusUI(status);

    if (status == STATUS_COMPLETE) {
        AfxMessageBox(
            _T("배달이 완료되었습니다!\n맛있게 드세요!\n\n주문내역에서 리뷰를 작성하실 수 있습니다."),
            MB_ICONINFORMATION);
        NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);

        // 배달 완료 후 리뷰 버튼 노출
        CWnd* pReviewBtn = GetDlgItem(IDC_BTN_WRITE_REVIEW);
        if (pReviewBtn) pReviewBtn->ShowWindow(SW_SHOW);

    } else if (status == STATUS_CANCELED) {
        AfxMessageBox(_T("주문이 취소되었습니다."), MB_ICONWARNING);
        NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    }
    return 0;
}

// ================================================================
//  RebuildInfoText  (기존 코드 그대로)
// ================================================================
void DeliveryOkDlg::RebuildInfoText()
{
    const TCHAR* labels[4];
    if (m_bDelivery)
        labels[0] = _T("주문접수"), labels[1] = _T("조리중"),
        labels[2] = _T("배달중"),   labels[3] = _T("배달완료");
    else
        labels[0] = _T("주문접수"), labels[1] = _T("조리중"),
        labels[2] = _T("준비완료"), labels[3] = _T("픽업완료");

    CString strInfo;

    if (m_bDelivery) {
        if (!m_strDeliveryAddr.IsEmpty())
            strInfo += _T("배달주소: ") + m_strDeliveryAddr + _T("\n");
        if (m_nDeliveryFee > 0) {
            CString strFee;
            strFee.Format(_T("배달비: %d원\n"), m_nDeliveryFee);
            strInfo += strFee;
        }
    } else {
        strInfo += _T("수령방법: 포장(직접 픽업)\n");
    }

    if (!m_strOrderDateTime.IsEmpty())
        strInfo += _T("주문시각: ") + m_strOrderDateTime + _T("\n");
    if (!m_strPayMethod.IsEmpty())
        strInfo += _T("결제수단: ") + m_strPayMethod + _T("\n");

    SetDlgItemText(IDC_STATIC_TIME, strInfo);
}

// ================================================================
//  UpdateStatusUI  (기존 코드 그대로)
// ================================================================
void DeliveryOkDlg::UpdateStatusUI(int status)
{
    const TCHAR* labels[4];
    if (m_bDelivery)
        labels[0] = _T("주문접수"), labels[1] = _T("조리중"),
        labels[2] = _T("배달중"),   labels[3] = _T("배달완료");
    else
        labels[0] = _T("주문접수"), labels[1] = _T("조리중"),
        labels[2] = _T("준비완료"), labels[3] = _T("픽업완료");

    CString strInfo;

    if (m_bDelivery) {
        if (!m_strDeliveryAddr.IsEmpty())
            strInfo += _T("배달주소: ") + m_strDeliveryAddr + _T("\n");
        if (m_nDeliveryFee > 0) {
            CString strFee;
            strFee.Format(_T("배달비: %d원\n"), m_nDeliveryFee);
            strInfo += strFee;
        }
    } else {
        strInfo += _T("수령방법: 포장(직접 픽업)\n");
    }

    if (!m_strOrderDateTime.IsEmpty())
        strInfo += _T("주문시각: ") + m_strOrderDateTime + _T("\n");
    if (!m_strPayMethod.IsEmpty())
        strInfo += _T("결제수단: ") + m_strPayMethod + _T("\n");

    CString strStatus;
    switch (status) {
    case STATUS_WAITING:
    case STATUS_PREPARING:
        strStatus = m_bDelivery
            ? CString() : CString();
        strStatus.Format(_T("예상 %s시간: 약 %d분"),
            m_bDelivery ? _T("배달") : _T("준비"),
            m_nEstimatedMinutes);
        break;
    case STATUS_DELIVERING:
        strStatus = m_bDelivery
            ? _T("라이더가 배달 중입니다.")
            : _T("준비가 완료되었습니다. 매장에서 픽업해 주세요.");
        break;
    case STATUS_COMPLETE:
        strStatus = m_bDelivery
            ? _T("배달 완료! 맛있게 드세요.")
            : _T("픽업 완료! 맛있게 드세요.");
        break;
    }
    strInfo += strStatus + _T("\n\n");

    CString bar;
    for (int i = 0; i < 4; ++i) {
        if (i < status)       bar += CString(_T("[V] ")) + labels[i] + _T("  ");
        else if (i == status) bar += CString(_T("[>] ")) + labels[i] + _T("  ");
        else                  bar += CString(_T("[ ] ")) + labels[i] + _T("  ");
    }
    strInfo += bar;

    SetDlgItemText(IDC_STATIC_TIME, strInfo);
}
