#pragma execution_character_set("utf-8")
#include "pch.h"
#include "Owner.h"
#include "afxdialogex.h"
#include "CSettingsDlg.h"
#include "resource.h"
#include "NetClient.h"
#include "Protocol.h"

using json = nlohmann::json;

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS_DIALOG, pParent)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
END_MESSAGE_MAP()

// 1. 창이 열릴 때 기존 설정 가져오기
BOOL CSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["owner_id"] = 1; // 🚨 임시 로그인 ID

	if (CNetClient::SendRequest(CmdOwner::REQ_GET_SETTINGS, req, res)) {
		if (res["status"] == Status::SUCCESS) {
			// 서버에서 온 데이터 파싱
			CString strName = CA2T(res.value("store_name", "").c_str(), CP_UTF8);
			CString strPhone = CA2T(res.value("phone", "").c_str(), CP_UTF8);
			CString strNotice = CA2T(res.value("notice", "").c_str(), CP_UTF8);

			CString strMinOrder, strDeliTip;
			strMinOrder.Format(_T("%d"), res.value("min_order", 0));
			strDeliTip.Format(_T("%d"), res.value("delivery_tip", 0));

			// 화면에 세팅
			SetDlgItemText(IDC_EDIT_STORE_NAME, strName);
			SetDlgItemText(IDC_EDIT_PHONE, strPhone);
			SetDlgItemText(IDC_EDIT_MIN_ORDER, strMinOrder);
			SetDlgItemText(IDC_EDIT_DELIVERY_TIP, strDeliTip);
			SetDlgItemText(IDC_EDIT_NOTICE, strNotice);
		}
	}
	return TRUE;
}

// 2. 저장(확인) 버튼 누를 때 서버로 업데이트
void CSettingsDlg::OnOK()
{
	CString strName, strPhone, strNotice, strMinOrder, strDeliTip;
	GetDlgItemText(IDC_EDIT_STORE_NAME, strName);
	GetDlgItemText(IDC_EDIT_PHONE, strPhone);
	GetDlgItemText(IDC_EDIT_MIN_ORDER, strMinOrder);
	GetDlgItemText(IDC_EDIT_DELIVERY_TIP, strDeliTip);
	GetDlgItemText(IDC_EDIT_NOTICE, strNotice);

	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["owner_id"] = 1; // 🚨 임시 로그인 ID
	req["store_name"] = std::string(CT2CA(strName, CP_UTF8));
	req["phone"] = std::string(CT2CA(strPhone, CP_UTF8));
	req["min_order"] = _ttoi(strMinOrder);
	req["delivery_tip"] = _ttoi(strDeliTip);
	req["notice"] = std::string(CT2CA(strNotice, CP_UTF8));

	if (CNetClient::SendRequest(CmdOwner::REQ_UPDATE_SETTINGS, req, res)) {
		if (res["status"] == Status::SUCCESS) {
			AfxMessageBox(_T("환경 설정이 저장되었습니다."));
			CDialogEx::OnOK(); // 창 닫기
		}
		else {
			AfxMessageBox(_T("저장에 실패했습니다."));
		}
	}
	else {
		AfxMessageBox(_T("서버 통신 오류!"));
	}
}