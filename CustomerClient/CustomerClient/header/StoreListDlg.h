#pragma once
#include "afxdialogex.h"


// StoreListDlg ダイアログ

class StoreListDlg : public CDialogEx
{
	DECLARE_DYNAMIC(StoreListDlg)

public:
	StoreListDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~StoreListDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STORELIST_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
