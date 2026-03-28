#pragma once
#include "afxdialogex.h"
#include "StoreInfo.h"

// StoreDetailDlg 대화 상자

class StoreDetailDlg : public CDialogEx
{
	DECLARE_DYNAMIC(StoreDetailDlg)

public:
	StoreDetailDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~StoreDetailDlg();

	CString m_strName;
	CString m_strAddr;
	CString m_strTime;
	CString m_strOff;
	CString m_strTel;

	StoreInfo m_storeInfo;   // ★ 추가

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STOREDETAIL_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedOk();

	DECLARE_MESSAGE_MAP()
};
