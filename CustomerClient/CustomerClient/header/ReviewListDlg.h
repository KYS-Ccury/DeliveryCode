#pragma once
#include "afxdialogex.h"
#include <string>
#include <vector>
#include <atomic>

#ifndef IDC_LIST_REVIEWS
#define IDC_LIST_REVIEWS            1980
#define IDC_STATIC_REVIEW_STORE     1981
#define IDC_STATIC_AVG_RATING       1982
#define IDC_BTN_WRITE_MY_REVIEW     1983
#endif

// 리뷰 1건 데이터
struct ReviewItem {
    int         reviewID  = 0;
    std::string authorID;
    int         rating    = 0;
    std::string content;
    std::string createdAt;
};

class ReviewListDlg : public CDialogEx
{
    DECLARE_DYNAMIC(ReviewListDlg)
public:
    ReviewListDlg(CWnd* pParent = nullptr);
    virtual ~ReviewListDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_REVIEW_LIST_DLG };
#endif

    int     m_nStoreID     = 0;
    CString m_strStoreName;
    bool    m_bCanWriteReview = false; // 이 가게에서 구매했으면 true

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnBack();
    afx_msg void    OnBnClickedWriteMyReview();
    afx_msg LRESULT OnReviewListResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl              m_listReviews;
    std::vector<ReviewItem> m_vecReviews;

    void LoadReviews();
    void RebuildUI();
    CString StarStr(int rating);  // rating → "★★★★☆"
};
