#pragma once
#include "afxdialogex.h"


// OrderHistoryDlg ダイアログ

class OrderHistoryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(OrderHistoryDlg)

public:
	OrderHistoryDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~OrderHistoryDlg();

	// 주문 데이터를 외부에서 넘겨받기 위한 변수들 (예시)
	CString m_strOrderNum;
	CString m_strShopName;
	int m_nTotalAmount;

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ORDERHISTORY_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

	// 버튼 클릭 이벤트
	afx_msg void OnBnClickedBtnBack();
	afx_msg void OnBnClickedBtnChat();

	DECLARE_MESSAGE_MAP()
};
