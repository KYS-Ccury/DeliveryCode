#pragma once
#include "afxdialogex.h"
#include "OrderInfo.h"
#include <vector>
#include <atomic>

// ================================================================
//  OrderHistoryDlg.h  ─  주문 현황 화면 (서버 연동 완성본)
//
//  [기능]
//  1. 진행 중인 주문 상태 실시간 표시 (NTF_ORDER_STATUS 수신)
//  2. 지난 주문 내역 목록 조회 (REQ_ORDER_HISTORY 203)
//  3. 1:1 채팅 버튼 → ChatDlg 실행
//  4. 주문 취소 버튼 (접수 대기 상태일 때만 활성화)
// ================================================================
class OrderHistoryDlg : public CDialogEx
{
    DECLARE_DYNAMIC(OrderHistoryDlg)
public:
    OrderHistoryDlg(CWnd* pParent = nullptr);
    virtual ~OrderHistoryDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ORDERHISTORY_DLG };
#endif

    // 외부에서 주입 (CartDlg → DeliveryOkDlg 대신 직접 열 때)
    CString m_strOrderNum;
    CString m_strShopName;
    int     m_nTotalAmount = 0;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedBtnChat();
    //afx_msg void    OnBnClickedBtnWriteReview();  // ★ 리뷰 작성 버튼

    // 서버 응답 핸들러
    afx_msg LRESULT OnOrderHistoryResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnOrderStatusPush(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    std::vector<OrderInfo> m_vecOrders;   // 주문 내역 캐시
    int                    m_nCurrentStatus = -1;

    void RequestOrderHistory();
    void UpdateStatusBar(int status);
    void PopulateOrderInfo(const OrderInfo& info);
};
