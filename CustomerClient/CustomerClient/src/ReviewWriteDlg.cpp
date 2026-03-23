// ReviewWriteDlg.cpp: 구현 파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ReviewWriteDlg.h"


// ReviewWriteDlg 대화 상자

IMPLEMENT_DYNAMIC(ReviewWriteDlg, CDialogEx)

ReviewWriteDlg::ReviewWriteDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_REVIEW_WRITE_DLG, pParent)
    , m_nStarRating(0) // 변수 초기화
    , m_strReviewText(_T("")) // 변수 초기화
{
}

ReviewWriteDlg::~ReviewWriteDlg()
{
}

void ReviewWriteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ReviewWriteDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_REVIEW_BACK, &ReviewWriteDlg::OnBnClickedBtnReviewBack)
	ON_BN_CLICKED(IDOK, &ReviewWriteDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// ReviewWriteDlg 메시지 처리기

BOOL ReviewWriteDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 별점 콤보박스 초기화
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_STAR_RATING);
    pCombo->AddString(_T("★★★★★ (5점)"));
    pCombo->AddString(_T("★★★★☆ (4점)"));
    pCombo->AddString(_T("★★★☆☆ (3점)"));
    pCombo->AddString(_T("★★☆☆☆ (2점)"));
    pCombo->AddString(_T("★☆☆☆☆ (1점)"));
    pCombo->SetCurSel(0); // 기본 5점 선택

    return TRUE;
}

void ReviewWriteDlg::OnBnClickedBtnReviewBack()
{
    EndDialog(IDCANCEL);
}

void ReviewWriteDlg::OnBnClickedOk()
{
    // 작성된 내용 변수에 저장
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_STAR_RATING);
    m_nStarRating = 5 - pCombo->GetCurSel(); // 인덱스 역순으로 점수 계산

    GetDlgItemText(IDC_EDIT_REVIEW_CONTENT, m_strReviewText);

    if (m_strReviewText.IsEmpty()) {
        AfxMessageBox(_T("리뷰 내용을 입력해 주세요!"));
        return;
    }

    CDialogEx::OnOK();
}