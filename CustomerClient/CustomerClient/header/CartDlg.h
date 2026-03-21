#pragma once
#include "afxdialogex.h"


// CartDlg ダイアログ

class CartDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CartDlg)

public:
	CartDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CartDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CART_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
