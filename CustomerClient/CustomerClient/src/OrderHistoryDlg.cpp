// OrderHistoryDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OrderHistoryDlg.h"


// OrderHistoryDlg ダイアログ

IMPLEMENT_DYNAMIC(OrderHistoryDlg, CDialogEx)

OrderHistoryDlg::OrderHistoryDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ORDERHISTORY_DLG, pParent)
{

}

OrderHistoryDlg::~OrderHistoryDlg()
{
}

void OrderHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(OrderHistoryDlg, CDialogEx)
END_MESSAGE_MAP()


// OrderHistoryDlg メッセージ ハンドラー
