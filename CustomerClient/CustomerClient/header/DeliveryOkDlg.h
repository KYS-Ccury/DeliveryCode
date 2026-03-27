#pragma once
// ================================================================
//  DeliveryOkDlg.h  (사장님 채팅 버튼 추가 버전)
//
//  변경 사항:
//    - IDC_BTN_CHAT (1806) 버튼 클릭 핸들러 추가
//      → ChatDlg 를 target_type="owner", order_id=m_nOrderId 로 열기
//    - m_nOrderId 필드 추가 (호출자가 주문 ID 설정)
//
//  resource.h 에서 이미 정의됨:
//    IDC_BTN_CHAT  1806  (기존 정의 재사용)
// ================================================================
#include "afxdialogex.h"
#include "OrderInfo.h"

#define WM_ORDER_STATUS_CHANGED (WM_USER + 201)

class DeliveryOkDlg : public CDialogEx
{
    DECLARE_DYNAMIC(DeliveryOkDlg)
public:
    DeliveryOkDlg(CWnd* pParent = nullptr);
    virtual ~DeliveryOkDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DELIVERY_OK_DLG };
#endif

    // ── 주문 기본 정보 ────────────────────────────────────────
    CString m_strOrderID;
    CString m_strStoreName;
    CString m_strOrderList;
    int     m_nTotalAmount      = 0;
    int     m_nEstimatedMinutes = 30;

    // ── 추가 정보 ─────────────────────────────────────────────
    CString m_strDeliveryAddr;
    CString m_strOrderDateTime;
    int     m_nUsedPoint    = 0;
    int     m_nDeliveryFee  = 0;
    CString m_strPayMethod;

    // ★ 사장님 채팅에 사용할 주문 ID (int)
    int     m_nOrderId      = 0;

    struct OrderLineItem {
        CString strMenuName;
        CString strOptions;
        int     nQuantity;
        int     nPrice;
    };
    std::vector<OrderLineItem> m_vecOrderLines;

    bool m_bDelivery = true;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedBtnChat();          // ★ 사장님 채팅
    afx_msg LRESULT OnOrderStatusChanged(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void UpdateStatusUI(int status);
    void RebuildInfoText();
};
