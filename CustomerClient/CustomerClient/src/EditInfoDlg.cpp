// ================================================================
//  EditInfoDlg.cpp  ─  내정보 수정 (비밀번호 변경)
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "EditInfoDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

static int EIJInt(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return -1;
    try { return std::stoi(j.substr(p + t.size())); } catch (...) { return -1; }
}

IMPLEMENT_DYNAMIC(EditInfoDlg, CDialogEx)

EditInfoDlg::EditInfoDlg(CWnd* pParent)
    : CDialogEx(IDD_EDITINFO_DLG, pParent)
{}

EditInfoDlg::~EditInfoDlg() {}

void EditInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(EditInfoDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,          &EditInfoDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BTN_BACK,  &EditInfoDlg::OnBnClickedBtnBack)
    ON_MESSAGE(WM_EDITINFO_RESPONSE, &EditInfoDlg::OnEditInfoResponse)
END_MESSAGE_MAP()

BOOL EditInfoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetErrorMsg(_T(""));

    // 서버 콜백 등록
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(
        CmdCustomer::REQ_CHANGE_PASSWORD,
        [hThis](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            ::PostMessage(hThis, WM_EDITINFO_RESPONSE, 0, (LPARAM)p);
        });

    return TRUE;
}

void EditInfoDlg::OnBnClickedOk()
{
    CString strCur, strNew, strNew2;
    GetDlgItemText(IDC_EDIT_CUR_PW,  strCur);
    GetDlgItemText(IDC_EDIT_NEW_PW,  strNew);
    GetDlgItemText(IDC_EDIT_NEW_PW2, strNew2);

    // ── 유효성 검사 ────────────────────────────────────────
    if (strCur.IsEmpty() || strNew.IsEmpty() || strNew2.IsEmpty()) {
        SetErrorMsg(_T("모든 항목을 입력해주세요."));
        return;
    }
    if (strNew != strNew2) {
        SetErrorMsg(_T("새 비밀번호가 일치하지 않습니다."));
        return;
    }
    if (strNew.GetLength() < 6) {
        SetErrorMsg(_T("비밀번호는 6자 이상이어야 합니다."));
        return;
    }

    // ── 서버 요청 ──────────────────────────────────────────
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        SetErrorMsg(_T("서버에 연결되어 있지 않습니다."));
        return;
    }

    std::string token   = AuthManager::GetInstance().GetAccessToken();
    std::string curPw   = CT2A(strCur,  CP_UTF8);
    std::string newPw   = CT2A(strNew,  CP_UTF8);
    std::string json = "{\"token\":\"" + token + "\","
                        "\"current_pw\":\"" + curPw + "\","
                        "\"new_pw\":\"" + newPw + "\"}";

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_CHANGE_PASSWORD, json);

    // 버튼 비활성화 (중복 전송 방지)
    CWnd* pOk = GetDlgItem(IDOK);
    if (pOk) pOk->EnableWindow(FALSE);
    SetErrorMsg(_T("처리 중..."));
}

LRESULT EditInfoDlg::OnEditInfoResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = EIJInt(*pBody, "status");
    delete pBody;

    NetworkManager::GetInstance()
        .UnregisterCallback(CmdCustomer::REQ_CHANGE_PASSWORD);

    CWnd* pOk = GetDlgItem(IDOK);
    if (pOk) pOk->EnableWindow(TRUE);

    if (status == (int)Status::SUCCESS) {
        AfxMessageBox(_T("비밀번호가 변경되었습니다."), MB_ICONINFORMATION);
        EndDialog(IDOK);
    } else {
        SetErrorMsg(_T("현재 비밀번호가 올바르지 않습니다."));
    }
    return 0;
}

void EditInfoDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance()
        .UnregisterCallback(CmdCustomer::REQ_CHANGE_PASSWORD);
    EndDialog(IDCANCEL);
}

void EditInfoDlg::SetErrorMsg(const CString& msg)
{
    CWnd* p = GetDlgItem(IDC_STATIC_EDITINFO_ERR);
    if (p) p->SetWindowText(msg);
}
