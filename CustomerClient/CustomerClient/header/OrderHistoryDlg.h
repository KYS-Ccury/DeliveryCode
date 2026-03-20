#pragma once
#include "afxdialogex.h"


// OrderHistoryDlg ダイアログ

class OrderHistoryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(OrderHistoryDlg)

public:
	OrderHistoryDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~OrderHistoryDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ORDERHISTORY_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
