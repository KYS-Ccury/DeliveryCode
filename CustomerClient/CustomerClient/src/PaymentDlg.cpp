// PaymentDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "PaymentDlg.h"


// PaymentDlg ダイアログ

IMPLEMENT_DYNAMIC(PaymentDlg, CDialogEx)

PaymentDlg::PaymentDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAYMENT_DLG, pParent)
{

}

PaymentDlg::~PaymentDlg()
{
}

void PaymentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(PaymentDlg, CDialogEx)
END_MESSAGE_MAP()


// PaymentDlg メッセージ ハンドラー
