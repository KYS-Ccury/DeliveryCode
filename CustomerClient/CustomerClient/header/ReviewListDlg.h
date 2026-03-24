#pragma once
#include "afxdialogex.h"

class ReviewListDlg : public CDialogEx
{
    DECLARE_DYNAMIC(ReviewListDlg)

public:
    ReviewListDlg(CWnd* pParent = nullptr);
    virtual ~ReviewListDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_REVIEW_LIST_DLG };
#endif

    // StoreListDlg 정보 버튼 또는 매장 화면에서 설정
    int     m_nStoreID     = 0;
    CString m_strStoreName;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnBnClickedBtnBack();

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl m_listReviews;
    void      LoadReviews();
};
