#pragma once
#include "afxdialogex.h"


// MenuDetailDlg ダイアログ

class MenuDetailDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MenuDetailDlg)

public:
	MenuDetailDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~MenuDetailDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IDD_MENUDETAIL_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
