#pragma once
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

    // ── 추가 정보 (신규) ──────────────────────────────────────
    CString m_strDeliveryAddr;   // 배달 주소
    CString m_strOrderDateTime;  // 주문 시각
    int     m_nUsedPoint    = 0; // 사용 포인트
    int     m_nDeliveryFee  = 0; // 배달비
    CString m_strPayMethod;      // 결제 수단명

    // 기존 m_strOrderList(단순 문자열) 대신 구조체 벡터로 교체
    struct OrderLineItem {
        CString strMenuName;
        CString strOptions;   // "옵션A +500, 옵션B +300" 형태
        int     nQuantity;
        int     nPrice;       // 해당 항목 소계
    };
    std::vector<OrderLineItem> m_vecOrderLines;  // 신규 추가

    bool m_bDelivery = true;  // false면 포장(픽업)
    int  m_nOrderId  = 0;    // 사장님 채팅용 주문 ID

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedBtnChat();
    afx_msg LRESULT OnOrderStatusChanged(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void UpdateStatusUI(int status);
    void RebuildInfoText();
};
