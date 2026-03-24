// ================================================================
//  SignupDlg.cpp  ─  회원가입 화면 완전 구현
//
//  [연동 프로토콜]
//    REQ (100): {
//      "id":"...", "pw":"...",
//      "name":"...", "phone":"...",
//      "address":"...", "role":1
//    }
//    role: 1=CUSTOMER, 2=OWNER, 3=RIDER
//
//    RES 성공: { "status":2000 }
//    RES 실패: { "status":4000, "message":"이미 사용 중인 아이디입니다." }
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "SignupDlg.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

// ── resource.h 에 없으면 임시 정의 ───────────────────────────
#ifndef IDD_SIGNUP_DLG
#define IDD_SIGNUP_DLG           1950
#define IDC_EDIT_SIGNUP_ID       1951
#define IDC_EDIT_SIGNUP_PW       1952
#define IDC_EDIT_SIGNUP_PW2      1953
#define IDC_EDIT_SIGNUP_NAME     1954
#define IDC_EDIT_SIGNUP_PHONE    1955
#define IDC_EDIT_SIGNUP_ADDR     1956
#define IDC_COMBO_SIGNUP_ROLE    1957
#define IDC_STATIC_SIGNUP_ERR    1958
#endif

#define WM_SIGNUP_RESPONSE (WM_USER + 102)

// ── JSON 헬퍼 ─────────────────────────────────────────────────
static int SJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); }
    catch (...) { return -1; }
}
static std::string SJStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static std::string SEscape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    return out;
}

// =================================================================

IMPLEMENT_DYNAMIC(SignupDlg, CDialogEx)

SignupDlg::SignupDlg(CWnd* pParent)
    : CDialogEx(IDD_SIGNUP_DLG, pParent)
{}
SignupDlg::~SignupDlg() {}

void SignupDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(SignupDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,              &SignupDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL,          &SignupDlg::OnBnClickedCancel)
    ON_MESSAGE(WM_SIGNUP_RESPONSE,   &SignupDlg::OnSignupResponse)
END_MESSAGE_MAP()

BOOL SignupDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 역할 콤보박스 항목 추가
    CWnd* pCombo = this->GetDlgItem(IDC_COMBO_SIGNUP_ROLE);
    if (pCombo) {
        CComboBox* pCB = static_cast<CComboBox*>(pCombo);
        pCB->AddString(_T("일반 고객"));   // index 0 → role 1
        pCB->AddString(_T("사장님"));      // index 1 → role 2
        pCB->AddString(_T("라이더"));      // index 2 → role 3
        pCB->SetCurSel(0);                 // 기본: 일반 고객
    }

    // 서버 연결 여부 안내
    if (!NetworkManager::GetInstance().IsConnected())
        SetError(_T("⚠ 서버 미연결 상태. 로그인 화면에서 먼저 연결하세요."));

    this->GetDlgItem(IDC_EDIT_SIGNUP_ID)->SetFocus();
    return FALSE;
}

// ── 에러 표시 ─────────────────────────────────────────────────
void SignupDlg::SetError(const CString& msg)
{
    CWnd* p = this->GetDlgItem(IDC_STATIC_SIGNUP_ERR);
    if (p) p->SetWindowText(msg);
}

// ── 입력 유효성 검사 ─────────────────────────────────────────
bool SignupDlg::ValidateInputs(CString& outID, CString& outPW,
                                CString& outName, CString& outPhone,
                                CString& outAddr, int& outRole)
{
    CString pw2;
    this->GetDlgItemText(IDC_EDIT_SIGNUP_ID,    outID);
    this->GetDlgItemText(IDC_EDIT_SIGNUP_PW,    outPW);
    this->GetDlgItemText(IDC_EDIT_SIGNUP_PW2,   pw2);
    this->GetDlgItemText(IDC_EDIT_SIGNUP_NAME,  outName);
    this->GetDlgItemText(IDC_EDIT_SIGNUP_PHONE, outPhone);
    this->GetDlgItemText(IDC_EDIT_SIGNUP_ADDR,  outAddr);

    outID.Trim(); outName.Trim(); outPhone.Trim(); outAddr.Trim();

    // ── 아이디 ────────────────────────────────────────────────
    if (outID.IsEmpty()) {
        SetError(_T("아이디를 입력하세요."));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_ID); if(_p) _p->SetFocus(); } return false;
    }
    if (outID.GetLength() < 4) {
        SetError(_T("아이디는 4자 이상이어야 합니다."));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_ID); if(_p) _p->SetFocus(); } return false;
    }

    // ── 비밀번호 ──────────────────────────────────────────────
    if (outPW.IsEmpty()) {
        SetError(_T("비밀번호를 입력하세요."));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_PW); if(_p) _p->SetFocus(); } return false;
    }
    if (outPW.GetLength() < 6) {
        SetError(_T("비밀번호는 6자 이상이어야 합니다."));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_PW); if(_p) _p->SetFocus(); } return false;
    }
    if (outPW != pw2) {
        SetError(_T("비밀번호가 일치하지 않습니다."));
        SetDlgItemText(IDC_EDIT_SIGNUP_PW2, _T(""));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_PW2); if(_p) _p->SetFocus(); } return false;
    }

    // ── 이름 ──────────────────────────────────────────────────
    if (outName.IsEmpty()) {
        SetError(_T("이름을 입력하세요."));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_NAME); if(_p) _p->SetFocus(); } return false;
    }

    // ── 전화번호: 숫자 9~11자 ────────────────────────────────
    CString phoneClean;
    for (int i = 0; i < outPhone.GetLength(); ++i)
        if (_istdigit(outPhone[i])) phoneClean += outPhone[i];
    if (phoneClean.GetLength() < 9 || phoneClean.GetLength() > 11) {
        SetError(_T("올바른 전화번호를 입력하세요. (예: 01012345678)"));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_PHONE); if(_p) _p->SetFocus(); } return false;
    }
    outPhone = phoneClean; // 하이픈 제거된 숫자만 전송

    // ── 주소 ──────────────────────────────────────────────────
    if (outAddr.IsEmpty()) {
        SetError(_T("주소를 입력하세요."));
        { CWnd* _p=this->GetDlgItem(IDC_EDIT_SIGNUP_ADDR); if(_p) _p->SetFocus(); } return false;
    }

    // ── 역할 ──────────────────────────────────────────────────
    CWnd* pCombo = this->GetDlgItem(IDC_COMBO_SIGNUP_ROLE);
    outRole = 1; // 기본: CUSTOMER
    if (pCombo) {
        int sel = static_cast<CComboBox*>(pCombo)->GetCurSel();
        outRole = (sel >= 0) ? sel + 1 : 1; // 0→1, 1→2, 2→3
    }

    SetError(_T(""));
    return true;
}

// ── 가입 완료 버튼 ────────────────────────────────────────────
void SignupDlg::OnBnClickedOk()
{
    if (m_bWaiting.load()) return;

    CString id, pw, name, phone, addr;
    int role = 1;
    if (!ValidateInputs(id, pw, name, phone, addr, role)) return;

    auto& net = NetworkManager::GetInstance();

    // ── 서버 미연결: 안내 후 중단 ────────────────────────────
    if (!net.IsConnected()) {
        AfxMessageBox(
            _T("서버에 연결되지 않아 회원가입을 진행할 수 없습니다.\n")
            _T("로그인 화면에서 서버에 먼저 연결해 주세요."),
            MB_ICONWARNING);
        return;
    }

    // ── 서버 전송 ─────────────────────────────────────────────
    net.RegisterCallback(CmdCommon::REQ_SIGNUP,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            this->PostMessage(WM_SIGNUP_RESPONSE, 0, (LPARAM)pBody);
        });

    m_bWaiting.store(true);
    this->GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("처리 중..."));

    std::string _id    = CT2A(id,    CP_UTF8); std::string sID    = SEscape(_id);
    std::string _pw    = CT2A(pw,    CP_UTF8); std::string sPW    = SEscape(_pw);
    std::string _name  = CT2A(name,  CP_UTF8); std::string sName  = SEscape(_name);
    std::string _phone = CT2A(phone, CP_UTF8); std::string sPhone = SEscape(_phone);
    std::string _addr  = CT2A(addr,  CP_UTF8); std::string sAddr  = SEscape(_addr);

    std::string json =
        "{\"id\":\""      + sID    + "\","
        "\"pw\":\""       + sPW    + "\","
        "\"name\":\""     + sName  + "\","
        "\"phone\":\""    + sPhone + "\","
        "\"address\":\""  + sAddr  + "\","
        "\"role\":"       + std::to_string(role) + "}";

    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_SIGNUP, json);
}

// ── 서버 응답 처리 ────────────────────────────────────────────
LRESULT SignupDlg::OnSignupResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);

    m_bWaiting.store(false);
    this->GetDlgItem(IDOK)->EnableWindow(TRUE);
    SetDlgItemText(IDOK, _T("가입 완료"));

    if (!pBody) return 0;

    int status = SJInt(*pBody, "status");

    if (status == (int)Status::SUCCESS) {
        delete pBody;
        AfxMessageBox(_T("회원가입이 완료되었습니다!\n로그인 화면으로 돌아갑니다."),
                      MB_ICONINFORMATION);
        // 아이디 저장 후 IDOK 종료 → LoginDlg에서 읽어 자동입력
        this->GetDlgItemText(IDC_EDIT_SIGNUP_ID, m_strResultID);
        NetworkManager::GetInstance().UnregisterCallback(CmdCommon::REQ_SIGNUP);
        CDialogEx::OnOK();
    } else {
        // 실패: message 필드 표시
        std::string msg = SJStr(*pBody, "message");
        delete pBody;

        CString errMsg;
        if (!msg.empty())
            errMsg = CA2T(msg.c_str(), CP_UTF8);
        else if (status == (int)Status::BAD_REQUEST)
            errMsg = _T("이미 사용 중인 아이디입니다.");
        else
            errMsg = _T("회원가입 중 오류가 발생했습니다.");

        SetError(errMsg);
        this->GetDlgItem(IDC_EDIT_SIGNUP_ID)->SetFocus();
    }
    return 0;
}

void SignupDlg::OnBnClickedCancel()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCommon::REQ_SIGNUP);
    CDialogEx::OnCancel();
}
