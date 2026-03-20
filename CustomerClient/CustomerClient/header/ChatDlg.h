#pragma once
#include "afxdialogex.h"


// ChatDlg ダイアログ

class ChatDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ChatDlg)

public:
	ChatDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~ChatDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHAT_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
