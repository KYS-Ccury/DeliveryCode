#pragma once
#include "afxdialogex.h"


// MainHomeDlg ダイアログ

class MainHomeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MainHomeDlg)

public:
	MainHomeDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~MainHomeDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAINHOME_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
