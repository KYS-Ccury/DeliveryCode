// StoreDetailDlg.cpp: 구현 파일
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "StoreDetailDlg.h"


// StoreDetailDlg 대화 상자

IMPLEMENT_DYNAMIC(StoreDetailDlg, CDialogEx)

StoreDetailDlg::StoreDetailDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STOREDETAIL_DLG, pParent)
{

}

StoreDetailDlg::~StoreDetailDlg()
{
}

void StoreDetailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    // 리소스 ID와 변수 연결
    DDX_Text(pDX, IDC_EDIT_STORE_NAME, m_strName);
    DDX_Text(pDX, IDC_EDIT_STORE_ADDR, m_strAddr);
    DDX_Text(pDX, IDC_EDIT_STORE_TIME, m_strTime);
    DDX_Text(pDX, IDC_EDIT_STORE_OFF, m_strOff);
    DDX_Text(pDX, IDC_EDIT_STORE_TEL, m_strTel);
}


BEGIN_MESSAGE_MAP(StoreDetailDlg, CDialogEx)
END_MESSAGE_MAP()


// StoreDetailDlg 메시지 처리기
