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
    CString m_strOrderID;
    CString m_strStoreName;
    CString m_strOrderList;
    int     m_nTotalAmount = 0;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void    OnBnClickedBtnWriteReview();
    afx_msg LRESULT OnOrderStatusChanged(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
private:
    void UpdateStatusUI(int status); // DeliveryStatus enum 값 받음
};
