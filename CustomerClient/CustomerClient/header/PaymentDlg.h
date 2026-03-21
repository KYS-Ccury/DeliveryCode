#pragma once
#include "afxdialogex.h"


// PaymentDlg ダイアログ

class PaymentDlg : public CDialogEx
{
	DECLARE_DYNAMIC(PaymentDlg)

public:
	PaymentDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~PaymentDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAYMENT_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
