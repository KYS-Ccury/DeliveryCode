#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ReviewListDlg.h"

IMPLEMENT_DYNAMIC(ReviewListDlg, CDialogEx)

ReviewListDlg::ReviewListDlg(CWnd* pParent)
    : CDialogEx(IDD_REVIEW_LIST_DLG, pParent) {}
ReviewListDlg::~ReviewListDlg() {}

void ReviewListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ReviewListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK, &ReviewListDlg::OnBnClickedBtnBack)
END_MESSAGE_MAP()

BOOL ReviewListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    LoadReviews();
    return TRUE;
}

void ReviewListDlg::OnBnClickedBtnBack()
{
    CDialogEx::OnCancel();
}

void ReviewListDlg::LoadReviews()
{
    // TODO: 서버에서 리뷰 목록 조회
}
