#pragma once
#include "afxdialogex.h"

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
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBtnReviewBack();
    afx_msg void OnBnClickedOk();
    DECLARE_MESSAGE_MAP()
};
