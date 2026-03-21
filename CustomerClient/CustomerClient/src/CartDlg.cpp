// CartDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "CartDlg.h"


// CartDlg ダイアログ

IMPLEMENT_DYNAMIC(CartDlg, CDialogEx)

CartDlg::CartDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CART_DLG, pParent)
{

}

CartDlg::~CartDlg()
{
}

void CartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CartDlg, CDialogEx)
END_MESSAGE_MAP()


// CartDlg メッセージ ハンドラー
