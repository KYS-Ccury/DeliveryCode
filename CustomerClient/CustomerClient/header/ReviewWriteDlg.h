#pragma once
#include "afxdialogex.h"


// ReviewWriteDlg 대화 상자

class ReviewWriteDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ReviewWriteDlg)

public:
	ReviewWriteDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~ReviewWriteDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REVIEW_WRITE_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnReviewBack();
	afx_msg void OnBnClickedOk(); // 확인 버튼 핸들러
	DECLARE_MESSAGE_MAP()

public:
	int m_nStarRating;      // 최종 선택된 별점
	CString m_strReviewText; // 최종 작성된 내용

};
