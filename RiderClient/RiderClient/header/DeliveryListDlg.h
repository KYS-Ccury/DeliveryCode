#pragma once
#include <afxdialogex.h>
#include <afxcmn.h>

struct OrderListItem {
    int     orderId     = 0;
    CString storeName;
    CString pickupAddr;
    CString destAddr;
    int     deliveryFee = 0;
    int     elapsedSec  = 0;
    CString rawPushData;
};

class DeliveryListDlg : public CDialogEx {
    DECLARE_DYNAMIC(DeliveryListDlg)
public:
    DeliveryListDlg(CWnd* pParent = nullptr);
    virtual ~DeliveryListDlg();
    enum { IDD = IDD_DELIVERY_LIST_DLG };
    void AddDispatchItem(const OrderListItem& item);
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_listOrders;
    CTabCtrl  m_tabList;
    CArray<OrderListItem, OrderListItem&> m_items;

    afx_msg void    OnBtnRefresh();
    afx_msg void    OnBtnAcceptItem();
    afx_msg void    OnBtnRejectItem();
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);

    void InitListCtrl();
    void RefreshCurrentList();
    void PopulateList();
    void PopulateHistoryList();
    void UpdateElapsedTime();
    void ParseOrderListResponse(const CString& payload);
    int  GetSelectedIndex() const;
};
