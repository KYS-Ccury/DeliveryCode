// ChatDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ChatDlg.h"


// ChatDlg ダイアログ

IMPLEMENT_DYNAMIC(ChatDlg, CDialogEx)

ChatDlg::ChatDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHAT_DLG, pParent)
{

}

ChatDlg::~ChatDlg()
{
}

void ChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ChatDlg, CDialogEx)
END_MESSAGE_MAP()


// ChatDlg メッセージ ハンドラー
