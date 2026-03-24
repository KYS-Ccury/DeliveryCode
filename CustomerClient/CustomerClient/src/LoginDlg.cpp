// ================================================================
//  LoginDlg.cpp  ─  로그인 화면 (수정본)
//
//  resource.h 에 IDC_EDIT_ID, IDC_EDIT_PW, IDC_BTN_SIGNUP 가 없으므로
//  → GetDlgItemText() 방식으로 직접 읽기
//  → 회원가입 버튼이 RC에 있으면 IDC 값을 resource.h에 추가 후 사용
//  → 없으면 IDOK 하나로만 처리 (현재 구조 유지)
//
//  [연동 프로토콜]
//    REQ : CmdCommon::REQ_LOGIN (101)
//          { "id":"...", "pw":"..." }
//    RES : { "status":2000, "token":"...", "login_id":"..." }
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "LoginDlg.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"

#define WM_LOGIN_RESPONSE (WM_USER + 100)

// ── 간이 JSON 헬퍼 ────────────────────────────────────────────
static std::string LoginJStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static int LoginJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); } catch (...) { return -1; }
}

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

LoginDlg::LoginDlg(CWnd* pParent)
    : CDialogEx(IDD_LOGIN_DLG, pParent)
    , m_bWaiting(false)
{}
LoginDlg::~LoginDlg() {}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    // RC에 정의된 에디트 ID가 있으면 아래 DDX 활성화:
    // DDX_Text(pDX, IDC_EDIT_ID, m_strID);
    // DDX_Text(pDX, IDC_EDIT_PW, m_strPW);
}

BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,           &LoginDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL,       &LoginDlg::OnBnClickedCancel)
    ON_MESSAGE(WM_LOGIN_RESPONSE, &LoginDlg::OnLoginResponse)
END_MESSAGE_MAP()

BOOL LoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 서버 연결
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        net.Connect("10.10.10.122", 9000);
        // 연결 실패해도 UI는 계속 표시 (오프라인 테스트 가능)
    }

    // 로그인 응답 콜백 등록
    net.RegisterCallback(CmdCommon::REQ_LOGIN,
        [this](uint16_t, const std::string& body) {
            int status = LoginJInt(body, "status");
            if (status == (int)Status::SUCCESS) {
                m_strToken   = LoginJStr(body, "token");
                m_strUserID  = LoginJStr(body, "login_id");
            }
            PostMessage(WM_LOGIN_RESPONSE, 0, (status == (int)Status::SUCCESS) ? 1 : 0);
        });

    return TRUE;
}

// ── 로그인 버튼 ───────────────────────────────────────────────
void LoginDlg::OnBnClickedOk()
{
    if (m_bWaiting) return;

    // ── ID/PW 읽기 ────────────────────────────────────────────
    // RC에 에디트 컨트롤 ID가 있으면 GetDlgItemText 사용
    // 없으면(현재 구조) 임시 하드코딩으로 진입 허용
    CString strID, strPW;

    // IDC_EDIT_ID, IDC_EDIT_PW가 RC에 정의되어 있으면:
    // GetDlgItemText(IDC_EDIT_ID, strID);
    // GetDlgItemText(IDC_EDIT_PW, strPW);
    // 없으면 아래 임시값 사용 (테스트용)
    strID = _T("test_user");
    strPW = _T("test1234");

    if (strID.IsEmpty() || strPW.IsEmpty()) {
        AfxMessageBox(_T("\uC544\uC774\uB514\uC640 \uBE44\uBC00\uBC88\uD638\uB97C \uC785\uB825\uD558\uC138\uC694."));
        return;
    }

    std::string id = CT2A(strID, CP_UTF8);
    std::string pw = CT2A(strPW, CP_UTF8);

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 서버 없으면 더미 로그인
        AuthManager::GetInstance().Login(id, pw);
        CDialogEx::OnOK();
        return;
    }

    m_bWaiting = true;
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("\uB85C\uADF8\uC778 \uC911..."));

    std::string json = "{\"id\":\"" + id + "\",\"pw\":\"" + pw + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_LOGIN, json);
}

// ── 서버 응답 처리 (UI 스레드) ────────────────────────────────
LRESULT LoginDlg::OnLoginResponse(WPARAM, LPARAM lParam)
{
    m_bWaiting = false;
    GetDlgItem(IDOK)->EnableWindow(TRUE);
    SetDlgItemText(IDOK, _T("\uB85C\uADF8\uC778"));

    if (lParam == 1) {
        // 성공
        CString strID = _T("test_user");
        // GetDlgItemText(IDC_EDIT_ID, strID);  // RC에 IDC 있으면 활성화

        AuthManager::GetInstance().Login(
            std::string(CT2A(strID, CP_UTF8)), "",
            m_strToken, m_strUserID);
        CDialogEx::OnOK();
    } else {
        AfxMessageBox(_T("\uC544\uC774\uB514 \uB610\uB294 \uBE44\uBC00\uBC88\uD638\uAC00 \uC62C\uBC14\uB974\uC9C0 \uC54A\uC2B5\uB2C8\uB2E4."), MB_ICONWARNING);
    }
    return 0;
}

void LoginDlg::OnBnClickedCancel()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCommon::REQ_LOGIN);
    CDialogEx::OnCancel();
}
