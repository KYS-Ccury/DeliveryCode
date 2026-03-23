// OrderHistoryDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "OrderHistoryDlg.h"

#include "ChatDlg.h" 

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
	ON_BN_CLICKED(IDC_BTN_BACK, &OrderHistoryDlg::OnBnClickedBtnBack)
	ON_BN_CLICKED(IDC_BTN_CHAT, &OrderHistoryDlg::OnBnClickedBtnChat)
END_MESSAGE_MAP()


// OrderHistoryDlg メッセージ ハンドラー

BOOL OrderHistoryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 1. 데이터 표시 (실제 데이터로 세팅)
    SetDlgItemText(IDC_STATIC_ORDER_NUM, _T("주문번호 : ") + m_strOrderNum);
    SetDlgItemText(IDC_STATIC_SHOP_NAME, _T("매장명 : ") + m_strShopName);

    CString strTotal;
    strTotal.Format(_T("총 결제금액 : %d원"), m_nTotalAmount);
    SetDlgItemText(IDC_STATIC_FINAL_TOTAL, strTotal);

    return TRUE;
}

void OrderHistoryDlg::OnBnClickedBtnBack()
{
    OnCancel(); // 이전 화면(장바구니 또는 메인)으로 돌아가기
}

void OrderHistoryDlg::OnBnClickedBtnChat()
{
    // 1. 채팅 다이얼로그 객체 생성
    ChatDlg dlg;

    // 2. (선택) 채팅방 상단에 표시될 매장 이름을 넘겨줄 수 있습니다.
    // dlg.m_strTargetName = m_strShopName; 

    // 3. 모달(Modal) 방식으로 채팅창 띄우기
    // 이 코드가 실행되면 채팅창이 닫히기 전까지 주문현황창은 잠시 대기합니다.
    dlg.DoModal();
}