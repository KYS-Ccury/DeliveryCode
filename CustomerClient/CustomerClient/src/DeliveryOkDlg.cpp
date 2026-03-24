// ================================================================
//  DeliveryOkDlg.cpp  ─  주문 완료 / 실시간 배달 현황 화면 (수정본)
//
//  기존 DeliveryOkDlg.h 기준으로 맞춤:
//    - WM_ORDER_STATUS_CHANGED  : 헤더에 이미 (WM_USER+201) 정의됨
//    - OnOrderStatusChanged()   : 헤더에 이미 선언됨
//    - UpdateStatusUI()         : 헤더에 이미 선언됨
//    - m_strStoreName, m_strOrderList, m_nTotalAmount : 헤더에 존재
//    - m_nEstimatedMinutes 추가 → 헤더에 직접 추가 불필요,
//      이 cpp에서 로컬 변수로 처리
//
//  resource.h 기준 IDC:
//    IDC_STATIC_STORE_NAME  1700
//    IDC_STATIC_ORDER_LIST  1701
//    IDC_STATIC_PRICE       1702
//    IDC_STATIC_TIME        1703
//    IDC_BTN_WRITE_REVIEW   1704
//    IDC_STATIC_TIME → 상태 표시 + 예상 시간 모두 여기 표시
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "DeliveryOkDlg.h"
#include "ReviewWriteDlg.h"
#include "OrderManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

IMPLEMENT_DYNAMIC(DeliveryOkDlg, CDialogEx)

DeliveryOkDlg::DeliveryOkDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_OK_DLG, pParent)
    , m_nTotalAmount(0)
    , m_nEstimatedMinutes(30)
{}
DeliveryOkDlg::~DeliveryOkDlg() {}

void DeliveryOkDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(DeliveryOkDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                    &DeliveryOkDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BTN_WRITE_REVIEW,    &DeliveryOkDlg::OnBnClickedBtnWriteReview)
    ON_MESSAGE(WM_ORDER_STATUS_CHANGED,    &DeliveryOkDlg::OnOrderStatusChanged)
END_MESSAGE_MAP()

BOOL DeliveryOkDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // ── 주문 정보 표시 ────────────────────────────────────────
    SetDlgItemText(IDC_STATIC_STORE_NAME, m_strStoreName);
    SetDlgItemText(IDC_STATIC_ORDER_LIST, m_strOrderList);

    CString strPrice;
    strPrice.Format(_T("%d원"), m_nTotalAmount);
    SetDlgItemText(IDC_STATIC_PRICE, strPrice);

    // ── 주문번호 표시 (IDC_STATIC_TIME 재활용 또는 윈도우 타이틀) ──
    CString strTitle;
    strTitle.Format(_T("주문완료 - %s"), (LPCTSTR)m_strOrderID);
    SetWindowText(strTitle);

    // ── 초기 상태: 접수 대기 ─────────────────────────────────
    UpdateStatusUI(STATUS_WAITING);
    GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(FALSE);

    // ── 서버 실시간 PUSH 콜백 등록 ───────────────────────────
    // NTF_ORDER_STATUS (210): 서버가 먼저 Push 하는 패킷
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::NTF_ORDER_STATUS,
        [hThis](uint16_t, const std::string& body) {
            // "status" 값 파싱
            std::string token = "\"status\":";
            auto pos = body.find(token);
            int status = 0;
            if (pos != std::string::npos) {
                try { status = std::stoi(body.substr(pos + token.size())); }
                catch (...) { status = 0; }
            }
            int* p = new int(status);
            ::PostMessage(hThis, WM_ORDER_STATUS_CHANGED, (WPARAM)p, 0);
        });

    // OrderManager 콜백도 유지 (로컬 테스트 호환)
    OrderManager::GetInstance().RegisterOrderStatusCallback(
        [hThis](const std::string&, int status, const std::string&) {
            int* p = new int(status);
            ::PostMessage(hThis, WM_ORDER_STATUS_CHANGED, (WPARAM)p, 0);
        });

    return TRUE;
}

// ── 주문 상태 변경 메시지 처리 ───────────────────────────────
LRESULT DeliveryOkDlg::OnOrderStatusChanged(WPARAM wParam, LPARAM)
{
    int* p = reinterpret_cast<int*>(wParam);
    int status = p ? *p : 0;
    delete p;

    UpdateStatusUI(status);

    if (status == STATUS_COMPLETE) {
        GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(TRUE);
        AfxMessageBox(_T("배달이 완료되었습니다!\n맛있게 드세요!"), MB_ICONINFORMATION);
        NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    } else if (status == STATUS_CANCELED) {
        AfxMessageBox(_T("주문이 취소되었습니다."), MB_ICONWARNING);
        NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    }
    return 0;
}

// ── 상태 단계 UI 갱신 ────────────────────────────────────────
// IDC_STATIC_TIME 하나로 진행 상태 + 예상 시간 모두 표시
void DeliveryOkDlg::UpdateStatusUI(int status)
{
    const TCHAR* labels[] = {
        _T("주문접수"),
        _T("조리중"),
        _T("배달중"),
        _T("배달완료")
    };

    CString s;
    for (int i = 0; i < 4; ++i) {
        if      (i < status)  s += CString(_T("[V] ")) + labels[i] + _T("  ");
        else if (i == status) s += CString(_T("[>] ")) + labels[i] + _T("  ");
        else                  s += CString(_T("[ ] ")) + labels[i] + _T("  ");
    }

    // 예상 시간 덧붙이기
    if (status == STATUS_WAITING || status == STATUS_PREPARING) {
        CString strEst;
        strEst.Format(_T("\n예상 배달 시간: 약 %d분"), m_nEstimatedMinutes);
        s += strEst;
    } else if (status == STATUS_DELIVERING) {
        s += _T("\n라이더가 배달 중입니다.");
    } else if (status == STATUS_COMPLETE) {
        s += _T("\n배달 완료!");
    }

    SetDlgItemText(IDC_STATIC_TIME, s);
}

// ── 리뷰 작성 버튼 ───────────────────────────────────────────
void DeliveryOkDlg::OnBnClickedBtnWriteReview()
{
    ReviewWriteDlg dlg(this);
    // ReviewWriteDlg 는 m_strOrderID 없음 → 전달 불필요
    if (dlg.DoModal() == IDOK) {
        AfxMessageBox(_T("리뷰가 등록되었습니다. 감사합니다!"), MB_ICONINFORMATION);
        GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(FALSE);
    }
}

// ── 홈으로 버튼 ──────────────────────────────────────────────
void DeliveryOkDlg::OnBnClickedOk()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    CDialogEx::OnOK();
}
