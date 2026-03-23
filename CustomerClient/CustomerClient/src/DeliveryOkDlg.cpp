// DeliveryOkDlg.cpp: 구현 파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "DeliveryOkDlg.h"

#include "ReviewWriteDlg.h" // 리뷰 작성 페이지

// DeliveryOkDlg 대화 상자

IMPLEMENT_DYNAMIC(DeliveryOkDlg, CDialogEx)

DeliveryOkDlg::DeliveryOkDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DELIVERY_OK_DLG, pParent)
{

}

DeliveryOkDlg::~DeliveryOkDlg()
{
}

void DeliveryOkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(DeliveryOkDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_WRITE_REVIEW, &DeliveryOkDlg::OnBnClickedBtnWriteReview)
END_MESSAGE_MAP()


// DeliveryOkDlg 메시지 처리기

BOOL DeliveryOkDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 실제로는 결제 모듈이나 장바구니 데이터에서 가져온 값을 세팅합니다.
    SetDlgItemText(IDC_STATIC_STORE_NAME, _T("맛있는 치킨 강남점"));
    SetDlgItemText(IDC_STATIC_ORDER_LIST, _T("후라이드 치킨 1개\n콜라 1.25L 1개"));
    SetDlgItemText(IDC_STATIC_PRICE, _T("21,000원"));
    SetDlgItemText(IDC_STATIC_TIME, _T("2026-03-23 12:40"));

    return TRUE;
}

void DeliveryOkDlg::OnBnClickedBtnWriteReview()
{
    // 리뷰 작성 다이얼로그 객체 생성
    ReviewWriteDlg dlg;

    // (선택) 리뷰 창에 어떤 매장에 대한 리뷰인지 정보를 미리 줄 수도 있습니다.
    // dlg.m_strTargetStore = _T("맛있는 치킨 강남점");

    // 다이얼로그 띄우기 (Modal)
    if (dlg.DoModal() == IDOK)
    {
        // 사용자가 '확인'을 눌러서 리뷰 작성을 완료했을 때의 처리
        // 예: 작성된 별점과 내용을 가져와서 서버에 저장하는 로직 등
        int rating = dlg.m_nStarRating;
        CString content = dlg.m_strReviewText;

        AfxMessageBox(_T("리뷰가 성공적으로 등록되었습니다!"));

        // 리뷰 작성 후에는 버튼을 비활성화하거나 창을 닫을 수 있습니다.
        GetDlgItem(IDC_BTN_WRITE_REVIEW)->EnableWindow(FALSE);
    }
}