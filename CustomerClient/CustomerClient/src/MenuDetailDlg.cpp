// MenuDetailDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MenuDetailDlg.h"


// MenuDetailDlg ダイアログ

IMPLEMENT_DYNAMIC(MenuDetailDlg, CDialogEx)

MenuDetailDlg::MenuDetailDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MENUDETAIL_DLG, pParent)
{

}

MenuDetailDlg::~MenuDetailDlg()
{
}

void MenuDetailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MenuDetailDlg, CDialogEx)
END_MESSAGE_MAP()


// MenuDetailDlg メッセージ ハンドラー
