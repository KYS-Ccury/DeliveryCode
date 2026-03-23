#pragma once
#include "afxdialogex.h"


// DeliveryOkDlg 대화 상자

class DeliveryOkDlg : public CDialogEx
{
	DECLARE_DYNAMIC(DeliveryOkDlg)

public:
	DeliveryOkDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~DeliveryOkDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DELIVERY_OK_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	BOOL OnInitDialog();

	afx_msg void OnBnClickedBtnWriteReview();

	DECLARE_MESSAGE_MAP()
};
