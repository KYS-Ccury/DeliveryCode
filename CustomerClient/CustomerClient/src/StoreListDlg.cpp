// StoreListDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "StoreListDlg.h"


// StoreListDlg ダイアログ

IMPLEMENT_DYNAMIC(StoreListDlg, CDialogEx)

StoreListDlg::StoreListDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STORELIST_DLG, pParent)
{

}

StoreListDlg::~StoreListDlg()
{
}

void StoreListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(StoreListDlg, CDialogEx)
END_MESSAGE_MAP()


// StoreListDlg メッセージ ハンドラー
