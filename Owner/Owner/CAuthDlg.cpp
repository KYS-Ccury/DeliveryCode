#include "pch.h"
#include "Owner.h" 
#include "CAuthDlg.h"
#include "Struct.h"
#include "NetClient.h"
#include "json.hpp"
#include "Protocol.h"

CString g_strOwnerID; // 전역 변수로 저장
CString g_strOwnerPW;

using json = nlohmann::json;

// ============================================================
// 🚨 [필수 추가] 다이얼로그 생성자 및 DoDataExchange 구현부
// 이 부분이 있어야 LNK2019 에러가 발생하지 않습니다!
// ============================================================
CLoginDlg::CLoginDlg(CWnd* pParent) : CDialogEx(IDD_LOGIN_DIALOG, pParent) {}
void CLoginDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }

CSignUpDlg::CSignUpDlg(CWnd* pParent) : CDialogEx(IDD_SIGNUP_DIALOG, pParent) {}
void CSignUpDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
// ============================================================

// ================= CLoginDlg (로그인) =================
BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_LOGIN, &CLoginDlg::OnBnClickedBtnLogin)
    ON_BN_CLICKED(IDC_BTN_GO_SIGNUP, &CLoginDlg::OnBnClickedBtnGoSignup)
END_MESSAGE_MAP()

void CLoginDlg::OnBnClickedBtnLogin() {
    CString strID, strPW;
    GetDlgItemText(IDC_EDIT_LOGIN_ID, strID);
    GetDlgItemText(IDC_EDIT_LOGIN_PW, strPW);

    if (strID.IsEmpty() || strPW.IsEmpty()) {
        MessageBox(_T("아이디와 비밀번호를 입력하세요."), _T("알림"), MB_ICONWARNING);
        return;
    }

    json req;
    req["id"] = std::string(CT2CA(strID));
    req["pw"] = std::string(CT2CA(strPW));
    req["client_type"] = (int)ClientType::OWNER; // 🚨 [추가] 서버에 사장님임을 알림

    json res;
    // CmdCommon::REQ_LOGIN (101) 사용
    if (CNetClient::SendRequest(CmdCommon::REQ_LOGIN, req, res)) {
        if (res.contains("status") && res["status"] == Status::SUCCESS) {
            g_strOwnerID = strID;
            g_strOwnerPW = strPW;
            EndDialog(IDOK); // 로그인 성공 -> 메인 화면 진입
           
        }
        else {
            MessageBox(_T("로그인에 실패했습니다."), _T("실패"), MB_ICONSTOP);
        }
    }
    else {
        MessageBox(_T("서버 연결 실패"), _T("에러"), MB_ICONERROR);
    }

}

void CLoginDlg::OnBnClickedBtnGoSignup() {
    CSignUpDlg dlg;
    dlg.DoModal();
}

// ================= CSignUpDlg (회원가입) =================
BEGIN_MESSAGE_MAP(CSignUpDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_DO_SIGNUP, &CSignUpDlg::OnBnClickedBtnDoSignup)
END_MESSAGE_MAP()

void CSignUpDlg::OnBnClickedBtnDoSignup() {
    CString strID, strPW, strPWConf, strName, strPhone, strAddr, strStore;
    GetDlgItemText(IDC_EDIT_SIGN_ID, strID);
    GetDlgItemText(IDC_EDIT_SIGN_PW, strPW);
    GetDlgItemText(IDC_EDIT_SIGN_PW_CONFIRM, strPWConf);
    GetDlgItemText(IDC_EDIT_SIGN_NAME, strName);
    GetDlgItemText(IDC_EDIT_SIGN_PHONE, strPhone);
    GetDlgItemText(IDC_EDIT_SIGN_ADDR, strAddr);
    GetDlgItemText(IDC_EDIT_SIGN_STORE, strStore);

    if (strID.IsEmpty() || strPW.IsEmpty() || strName.IsEmpty()) {
        MessageBox(_T("필수 정보를 입력하세요."), _T("안내"), MB_ICONWARNING);
        return;
    }
    if (strPW != strPWConf) {
        MessageBox(_T("비밀번호가 일치하지 않습니다."), _T("경고"), MB_ICONERROR);
        return;
    }

    json req;
    req["id"] = std::string(CT2CA(strID));
    req["pw"] = std::string(CT2CA(strPW));
    req["name"] = std::string(CT2CA(strName));
    req["phone"] = std::string(CT2CA(strPhone));
    req["address"] = std::string(CT2CA(strAddr));
    req["store_name"] = std::string(CT2CA(strStore));
    req["client_type"] = (int)ClientType::OWNER; // 🚨 [추가] 사장님 가입임을 명시

    json res;
    // CmdCommon::REQ_SIGNUP (100) 사용
    if (CNetClient::SendRequest(CmdCommon::REQ_SIGNUP, req, res)) {
        if (res.contains("status") && res["status"] == Status::SUCCESS) {
            MessageBox(_T("가입되었습니다!"), _T("성공"), MB_ICONINFORMATION);
            EndDialog(IDOK);
        }
        else {
            MessageBox(_T("가입 실패 사유: ") + CString(res["message"].get<std::string>().c_str()));
        }
    }
    else {
        MessageBox(_T("서버 전송 실패"), _T("에러"), MB_ICONERROR);
    }
}