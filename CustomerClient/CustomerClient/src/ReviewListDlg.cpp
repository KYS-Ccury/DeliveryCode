// ReviewListDlg.cpp: 구현 파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ReviewListDlg.h"


// ReviewListDlg 대화 상자

IMPLEMENT_DYNAMIC(ReviewListDlg, CDialogEx)

ReviewListDlg::ReviewListDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REVIEW_LIST_DLG, pParent)
{

}

ReviewListDlg::~ReviewListDlg()
{
}

void ReviewListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ReviewListDlg, CDialogEx)
END_MESSAGE_MAP()


// ReviewListDlg 메시지 처리기
