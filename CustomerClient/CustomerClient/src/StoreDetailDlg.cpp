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
    ON_BN_CLICKED(IDOK, &StoreDetailDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// StoreDetailDlg 메시지 처리기

BOOL StoreDetailDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // m_storeInfo 에 데이터가 있으면 각 필드에 채운다.
    // 데이터가 없으면 m_strXxx 멤버를 그대로 사용 (DoDataExchange 로 이미 바인딩됨)
    if (m_storeInfo.storeID > 0 || !m_storeInfo.storeName.empty())
    {
        auto toCS = [](const std::string& s) -> CString {
            return s.empty() ? _T("-") : CA2T(s.c_str(), CP_UTF8);
        };

        SetDlgItemText(IDC_EDIT_STORE_NAME, toCS(m_storeInfo.storeName));
        SetDlgItemText(IDC_EDIT_STORE_ADDR, toCS(m_storeInfo.address));
        SetDlgItemText(IDC_EDIT_STORE_TIME, toCS(m_storeInfo.openTime));
        SetDlgItemText(IDC_EDIT_STORE_OFF,  toCS(m_storeInfo.holiday));
        SetDlgItemText(IDC_EDIT_STORE_TEL,  toCS(m_storeInfo.phoneNumber));
    }

    // 모든 EditBox 읽기 전용으로 설정
    for (int id : { IDC_EDIT_STORE_NAME, IDC_EDIT_STORE_ADDR,
                    IDC_EDIT_STORE_TIME, IDC_EDIT_STORE_OFF, IDC_EDIT_STORE_TEL })
    {
        CWnd* p = GetDlgItem(id);
        if (p) p->EnableWindow(FALSE);
    }

    return TRUE;
}

void StoreDetailDlg::OnBnClickedOk()
{
    EndDialog(IDOK);
}
