#pragma once
// ================================================================
//  OrderListDlg.h  ─  주문 내역 목록 화면
//
//  [기능]
//  1. 전체 주문 내역 목록 조회 (REQ_ORDER_HISTORY 203)
//  2. 목록에서 항목 선택 시 상세 정보 표시
//  3. 배달 완료 주문에 대해 리뷰 작성 가능
//  4. 주문 상태별 색상 표시
//
//  [연동 프로토콜]
//  ▶ 주문 내역 조회
//    REQ (203): { "token":"..." }
//    RES: { "status":2000, "orders":[
//      { "order_id":"20240522-0001",
//        "store_id":101, "store_name":"황금치킨",
//        "order_datetime":"2024-05-22 12:30",
//        "total_payment":21000, "status":3,
//        "delivery_method":"배달",
//        "items":[{"menu_name":"황금치킨","quantity":1,"price":18000}]
//      }, ...
//    ]}
//
//  resource.h 사용 IDC:
//    IDC_BTN_BACK                1400
//    IDC_LIST_ORDER_HISTORY      2100
//    IDC_STATIC_OL_ORDER_NUM     2101
//    IDC_STATIC_OL_STORE_NAME    2102
//    IDC_STATIC_OL_DATETIME      2103
//    IDC_STATIC_OL_TOTAL         2104
//    IDC_STATIC_OL_STATUS        2105
//    IDC_LIST_OL_ITEMS           2106
//    IDC_STATIC_OL_EMPTY         2107
//    IDC_BTN_OL_WRITE_REVIEW     2108
//    IDC_STATIC_OL_METHOD        2109
// ================================================================
#include "afxdialogex.h"
#include "OrderInfo.h"
#include <vector>

class OrderListDlg : public CDialogEx
{
    DECLARE_DYNAMIC(OrderListDlg)
public:
    OrderListDlg(CWnd* pParent = nullptr);
    virtual ~OrderListDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ORDERLIST_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnWriteReview();
    afx_msg void    OnNMClickListOrderHistory(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnOrderHistoryResponse(WPARAM wParam, LPARAM lParam);
    afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void    OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    // ── 데이터 ──────────────────────────────────────────────
    std::vector<OrderInfo> m_vecOrders;   // 전체 주문 내역
    int                    m_nSelectedIdx = -1; // 현재 선택된 항목 인덱스

    // ── 컨트롤 ──────────────────────────────────────────────
    CListCtrl m_listHistory;  // 주문 목록
    CListBox  m_listItems;    // 선택 주문의 메뉴 목록
    CBrush    m_brushBg;

    CFont m_fontBold;   // 강조용 폰트 (버튼 등)
    CFont m_fontTitle;  // 제목용 큰 폰트 (금액 등)

    // ── 내부 메서드 ─────────────────────────────────────────
    void RequestOrderHistory();
    void RebuildOrderListUI();
    void PopulateDetailPanel(int index);
    void ClearDetailPanel();
    CString GetStatusText(int status) const;
    COLORREF GetStatusColor(int status) const;
};