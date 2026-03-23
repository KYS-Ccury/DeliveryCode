#pragma once
#include "afxdialogex.h"


// ReviewListDlg 대화 상자

class ReviewListDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ReviewListDlg)

public:
	ReviewListDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~ReviewListDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REVIEW_LIST_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
};
