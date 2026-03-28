#pragma once
#include "afxdialogex.h"
#include <atomic>

// ================================================================
//  ReviewWriteDlg.h  ─  리뷰 작성 화면 (수정본)
//  [변경]
//  - WaitForSingleObject 블로킹 제거
//  - OnReviewResponse() 핸들러 추가 (PostMessage 패턴)
//  - m_bWaiting : 중복 전송 방지
// ================================================================
class ReviewWriteDlg : public CDialogEx
{
    DECLARE_DYNAMIC(ReviewWriteDlg)
public:
    ReviewWriteDlg(CWnd* pParent = nullptr);
    virtual ~ReviewWriteDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_REVIEW_WRITE_DLG };
#endif

    int     m_nStarRating;
    CString m_strReviewText;
    int     m_nStoreID  = 0;   // 리뷰 대상 가게 ID (ReviewListDlg 경로용)
    int     m_nOrderID  = 0;   // ★ 서버 필수값: 배달완료 주문 ID (OrderListDlg 경로용)

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnReviewBack();
    afx_msg void    OnBnClickedOk();
    afx_msg LRESULT OnReviewResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    std::atomic<bool> m_bWaiting{ false };
};
