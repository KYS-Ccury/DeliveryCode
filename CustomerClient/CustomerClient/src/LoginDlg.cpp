// LoginDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "LoginDlg.h"


// LoginDlg ダイアログ

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

LoginDlg::LoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN_DLG, pParent)
{

}

LoginDlg::~LoginDlg()
{
}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
END_MESSAGE_MAP()


// LoginDlg メッセージ ハンドラー
