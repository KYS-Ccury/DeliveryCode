#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "DeliveryOkDlg.h"
#include "OrderManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"
#include "ChatDlg.h"

IMPLEMENT_DYNAMIC(DeliveryOkDlg, CDialogEx)

DeliveryOkDlg::DeliveryOkDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_OK_DLG, pParent)
    , m_nTotalAmount(0)
    , m_nEstimatedMinutes(30)
    , m_nUsedPoint(0)
    , m_nDeliveryFee(0)
    , m_nOrderId(0)
{}
DeliveryOkDlg::~DeliveryOkDlg() {}

void DeliveryOkDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(DeliveryOkDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                 &DeliveryOkDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BTN_CHAT,         &DeliveryOkDlg::OnBnClickedBtnChat)
    ON_MESSAGE(WM_ORDER_STATUS_CHANGED, &DeliveryOkDlg::OnOrderStatusChanged)
END_MESSAGE_MAP()

BOOL DeliveryOkDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CString strTitle;
    strTitle.Format(_T("주문완료 - %s"), (LPCTSTR)m_strOrderID);
    SetWindowText(strTitle);

    SetDlgItemText(IDC_STATIC_STORE_NAME, m_strStoreName);
    SetDlgItemText(IDC_STATIC_ORDER_LIST, m_strOrderList);

    RebuildInfoText();

    CWnd* pReviewBtn = GetDlgItem(IDC_BTN_WRITE_REVIEW);
    if (pReviewBtn) pReviewBtn->ShowWindow(SW_HIDE);

    // 채팅 버튼 표시
    CWnd* pChatBtn = GetDlgItem(IDC_BTN_CHAT);
    if (pChatBtn) pChatBtn->ShowWindow(SW_SHOW);

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

//void DeliveryOkDlg::RebuildInfoText()
//{
//    CString strPrice;
//    if (m_nUsedPoint > 0)
//        strPrice.Format(_T("%d원  (포인트 %d원 사용)"), m_nTotalAmount, m_nUsedPoint);
//    else
//        strPrice.Format(_T("%d원"), m_nTotalAmount);
//    SetDlgItemText(IDC_STATIC_PRICE, strPrice);
//}

void DeliveryOkDlg::RebuildInfoText()
{
    // ── 주문 항목 (옵션 포함) ─────────────────────────────
    CString strItems;
    if (!m_vecOrderLines.empty())
    {
        for (const auto& line : m_vecOrderLines)
        {
            CString strLine;
            strLine.Format(_T("  · %s x%d  %d원"),
                (LPCTSTR)line.strMenuName, line.nQuantity, line.nPrice);
            strItems += strLine + _T("\n");

            if (!line.strOptions.IsEmpty())
                strItems += _T("    [") + line.strOptions + _T("]\n");
        }
    }
    else
    {
        // 구버전 호환: 단순 문자열 그대로 사용
        strItems = m_strOrderList;
    }
    SetDlgItemText(IDC_STATIC_ORDER_LIST, strItems);

    // ── 금액 ─────────────────────────────────────────────
    CString strPrice;
    if (m_nUsedPoint > 0)
        strPrice.Format(_T("%d원  (포인트 %d원 사용)"), m_nTotalAmount, m_nUsedPoint);
    else
        strPrice.Format(_T("%d원"), m_nTotalAmount);
    SetDlgItemText(IDC_STATIC_PRICE, strPrice);
}

void DeliveryOkDlg::UpdateStatusUI(int status)
{
    //const TCHAR* labels[] = { _T("주문접수"), _T("조리중"), _T("배달중"), _T("배달완료") };
    // ── 상태 레이블: 포장이면 4단계 중 "배달중" → "준비완료"로 교체
    const TCHAR* labels[4];
    if (m_bDelivery)
        labels[0] = _T("주문접수"), labels[1] = _T("조리중"),
        labels[2] = _T("배달중"), labels[3] = _T("배달완료");
    else
        labels[0] = _T("주문접수"), labels[1] = _T("조리중"),
        labels[2] = _T("준비완료"), labels[3] = _T("픽업완료");   // ★ 포장

    CString strInfo;

    //if (!m_strDeliveryAddr.IsEmpty())
    //    strInfo += _T("배달주소: ") + m_strDeliveryAddr + _T("\n");
    //if (!m_strOrderDateTime.IsEmpty())
    //    strInfo += _T("주문시각: ") + m_strOrderDateTime + _T("\n");
    //if (!m_strPayMethod.IsEmpty())
    //    strInfo += _T("결제수단: ") + m_strPayMethod + _T("\n");
    //if (m_nDeliveryFee > 0) {
    //    CString strFee; strFee.Format(_T("배달비: %d원\n"), m_nDeliveryFee);
    //    strInfo += strFee;
    //}
    
    if (m_bDelivery) {
        // 배달: 주소/배달비 표시
        if (!m_strDeliveryAddr.IsEmpty())
            strInfo += _T("배달주소: ") + m_strDeliveryAddr + _T("\n");
        if (m_nDeliveryFee > 0) {
            CString strFee;
            strFee.Format(_T("배달비: %d원\n"), m_nDeliveryFee);
            strInfo += strFee;
        }
    }
    else {
        // 포장: 픽업 안내 표시
        strInfo += _T("수령방법: 포장(직접 픽업)\n");            // ★ 포장
    }

    if (!m_strOrderDateTime.IsEmpty())
        strInfo += _T("주문시각: ") + m_strOrderDateTime + _T("\n");
    if (!m_strPayMethod.IsEmpty())
        strInfo += _T("결제수단: ") + m_strPayMethod + _T("\n");

    // ── 상태별 안내 문구 ──────────────────────────────────────
    CString strStatus;
    switch (status) {
    case STATUS_WAITING:
    case STATUS_PREPARING:
        if (m_bDelivery)
            strStatus.Format(_T("예상 배달시간: 약 %d분"), m_nEstimatedMinutes);
        else
            strStatus.Format(_T("예상 준비시간: 약 %d분"), m_nEstimatedMinutes); // ★ 포장
        break;
    case STATUS_DELIVERING:
        strStatus = m_bDelivery
            ? _T("라이더가 배달 중입니다.")
            : _T("준비가 완료되었습니다. 매장에서 픽업해 주세요.");             // ★ 포장
        break;
    case STATUS_COMPLETE:
        strStatus = m_bDelivery
            ? _T("배달 완료! 맛있게 드세요.")
            : _T("픽업 완료! 맛있게 드세요.");                                 // ★ 포장
        break;
    }
    strInfo += strStatus + _T("\n\n");

    // ── 진행 상태 바 ──────────────────────────────────────────
    CString bar;
    for (int i = 0; i < 4; ++i) {
        if (i < status)  bar += CString(_T("[V] ")) + labels[i] + _T("  ");
        else if (i == status) bar += CString(_T("[>] ")) + labels[i] + _T("  ");
        else                  bar += CString(_T("[ ] ")) + labels[i] + _T("  ");
    }
    strInfo += bar;

    SetDlgItemText(IDC_STATIC_TIME, strInfo);
}

LRESULT DeliveryOkDlg::OnOrderStatusChanged(WPARAM wParam, LPARAM)
{
    int* p = reinterpret_cast<int*>(wParam);
    int status = p ? *p : 0;
    delete p;
    UpdateStatusUI(status);
    if (status == STATUS_COMPLETE) {
        AfxMessageBox(_T("배달이 완료되었습니다!\n맛있게 드세요!\n\n주문내역에서 리뷰를 작성하실 수 있습니다."), MB_ICONINFORMATION);
        NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    } else if (status == STATUS_CANCELED) {
        AfxMessageBox(_T("주문이 취소되었습니다."), MB_ICONWARNING);
        NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    }
    return 0;
}

void DeliveryOkDlg::OnBnClickedBtnChat()
{
    ChatDlg dlg(this);
    dlg.m_strTargetName = m_strStoreName.IsEmpty() ? _T("가게 문의") : m_strStoreName + _T(" 문의");
    dlg.m_strTargetType = _T("owner");
    dlg.m_nOrderId      = m_nOrderId;
    dlg.DoModal();
}

void DeliveryOkDlg::OnBnClickedOk()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::NTF_ORDER_STATUS);
    CDialogEx::OnOK();
}
