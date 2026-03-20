// MainHomeDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MainHomeDlg.h"


// MainHomeDlg ダイアログ

IMPLEMENT_DYNAMIC(MainHomeDlg, CDialogEx)

MainHomeDlg::MainHomeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAINHOME_DLG, pParent)
{

}

MainHomeDlg::~MainHomeDlg()
{
}

void MainHomeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MainHomeDlg, CDialogEx)
END_MESSAGE_MAP()


// MainHomeDlg メッセージ ハンドラー
