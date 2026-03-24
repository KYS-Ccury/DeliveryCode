// ================================================================
//  LoginDlg.cpp  ─  로그인 + 서버 연결 + 회원가입 전환 (완전판)
//
//  [연동 프로토콜]
//    로그인  REQ (101): { "id":"...", "pw":"..." }
//            RES      : { "status":2000, "token":"...", "login_id":"..." }
//            실패     : { "status":4001, "message":"..." }
//
//  [서버 연결 흐름]
//    OnBnClickedConnect → 별도 스레드에서 NetworkManager::Connect()
//    → PostMessage(WM_CONNECT_RESULT) → OnConnectResult → UI 갱신
//
//  [IDC 목록] ← resource.h 에 추가 필요
//    IDC_EDIT_LOGIN_ID       1900
//    IDC_EDIT_LOGIN_PW       1901
//    IDC_BTN_GOTO_SIGNUP     1902
//    IDC_STATIC_CONN_STATUS  1903
//    IDC_BTN_CONNECT         1904
//    IDC_EDIT_SERVER_IP      1905
//    IDC_EDIT_SERVER_PORT    1906
//    IDC_STATIC_LOGIN_ERR    1907
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "LoginDlg.h"
#include "SignupDlg.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "common/header/Types.h"
#include <thread>

// ── resource.h 에 없으면 임시 정의 ───────────────────────────
#ifndef IDC_EDIT_LOGIN_ID
#define IDC_EDIT_LOGIN_ID       1900
#define IDC_EDIT_LOGIN_PW       1901
#define IDC_BTN_GOTO_SIGNUP     1902
#define IDC_STATIC_CONN_STATUS  1903
#define IDC_BTN_CONNECT         1904
#define IDC_EDIT_SERVER_IP      1905
#define IDC_EDIT_SERVER_PORT    1906
#define IDC_STATIC_LOGIN_ERR    1907
#endif

// ── 커스텀 메시지 ─────────────────────────────────────────────
#define WM_LOGIN_RESPONSE  (WM_USER + 100)
#define WM_CONNECT_RESULT  (WM_USER + 101)   // lParam: 1=성공, 0=실패

// ── 기본 서버 설정 ────────────────────────────────────────────
static const TCHAR* DEFAULT_SERVER_IP   = _T("10.10.10.122");
static const TCHAR* DEFAULT_SERVER_PORT = _T("8080");

// ── 간이 JSON 파싱 ────────────────────────────────────────────
static std::string LJStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static int LJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); }
    catch (...) { return -1; }
}

// =================================================================

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

LoginDlg::LoginDlg(CWnd* pParent)
    : CDialogEx(IDD_LOGIN_DLG, pParent)
{}
LoginDlg::~LoginDlg() {}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                 &LoginDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL,             &LoginDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BTN_CONNECT,      &LoginDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_GOTO_SIGNUP,  &LoginDlg::OnBnClickedGotoSignup)
    ON_MESSAGE(WM_LOGIN_RESPONSE,       &LoginDlg::OnLoginResponse)
    ON_MESSAGE(WM_CONNECT_RESULT,       &LoginDlg::OnConnectResult)
END_MESSAGE_MAP()

// ── 초기화 ────────────────────────────────────────────────────
BOOL LoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 기본 서버 IP/Port 설정
    SetDlgItemText(IDC_EDIT_SERVER_IP,   DEFAULT_SERVER_IP);
    SetDlgItemText(IDC_EDIT_SERVER_PORT, DEFAULT_SERVER_PORT);

    // 로그인 버튼 초기 텍스트
    SetDlgItemText(IDOK, _T("로그인"));

    // 현재 연결 상태 반영
    UpdateConnStatusUI();

    // 이미 연결돼 있으면 연결 버튼 비활성화
    if (NetworkManager::GetInstance().IsConnected())
        this->GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);

    // 로그인 응답 콜백 등록
    NetworkManager::GetInstance().RegisterCallback(CmdCommon::REQ_LOGIN,
        [this](uint16_t, const std::string& body) {
            // ReceiveLoop 스레드 → UI 스레드
            int status = LJInt(body, "status");
            if (status == (int)Status::SUCCESS) {
                m_strToken        = LJStr(body, "token");
                m_strServerUserID = LJStr(body, "login_id");
            }
            // body 힙 복사해서 lParam으로 전달
            std::string* pBody = new std::string(body);
            this->PostMessage(WM_LOGIN_RESPONSE, 0, (LPARAM)pBody);
        });

    // 아이디 에디트에 포커스
    this->GetDlgItem(IDC_EDIT_LOGIN_ID)->SetFocus();
    return FALSE; // 포커스 직접 설정
}

// ── 연결 상태 UI 갱신 ────────────────────────────────────────
void LoginDlg::UpdateConnStatusUI()
{
    CWnd* pStatus = this->GetDlgItem(IDC_STATIC_CONN_STATUS);
    CWnd* pBtn    = this->GetDlgItem(IDC_BTN_CONNECT);
    if (!pStatus) return;

    if (NetworkManager::GetInstance().IsConnected()) {
        // 서버 IP 읽어서 표시
        CString strIP;
        this->GetDlgItemText(IDC_EDIT_SERVER_IP, strIP);
        CString msg;
        msg.Format(_T("● 연결됨  [%s]"), (LPCTSTR)strIP);
        pStatus->SetWindowText(msg);
        // 초록 텍스트: WM_CTLCOLORSTATIC 으로 처리하거나 라벨 텍스트로만 표시
        if (pBtn) pBtn->EnableWindow(FALSE);
    } else {
        pStatus->SetWindowText(_T("● 연결 안됨"));
        if (pBtn) pBtn->EnableWindow(TRUE);
    }
}

// ── 에러 메시지 표시 ─────────────────────────────────────────
void LoginDlg::SetLoginError(const CString& msg)
{
    CWnd* pErr = this->GetDlgItem(IDC_STATIC_LOGIN_ERR);
    if (pErr) pErr->SetWindowText(msg);
}

// ── 로그인 입력값 읽기 ───────────────────────────────────────
bool LoginDlg::ReadLoginFields(CString& id, CString& pw)
{
    this->GetDlgItemText(IDC_EDIT_LOGIN_ID, id);
    this->GetDlgItemText(IDC_EDIT_LOGIN_PW, pw);
    id.Trim();
    if (id.IsEmpty()) {
        SetLoginError(_T("아이디를 입력하세요."));
        this->GetDlgItem(IDC_EDIT_LOGIN_ID)->SetFocus();
        return false;
    }
    if (pw.IsEmpty()) {
        SetLoginError(_T("비밀번호를 입력하세요."));
        this->GetDlgItem(IDC_EDIT_LOGIN_PW)->SetFocus();
        return false;
    }
    return true;
}

// ── 서버 연결 버튼 ────────────────────────────────────────────
void LoginDlg::OnBnClickedConnect()
{
    if (m_bConnecting.load()) return;

    CString strIP, strPort;
    this->GetDlgItemText(IDC_EDIT_SERVER_IP,   strIP);
    this->GetDlgItemText(IDC_EDIT_SERVER_PORT, strPort);
    strIP.Trim();

    if (strIP.IsEmpty()) {
        SetLoginError(_T("서버 IP를 입력하세요."));
        return;
    }
    int port = _ttoi(strPort);
    if (port <= 0 || port > 65535) {
        SetLoginError(_T("올바른 포트 번호를 입력하세요. (1~65535)"));
        return;
    }

    // 버튼/에디트 비활성화
    m_bConnecting.store(true);
    this->GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
    SetDlgItemText(IDC_BTN_CONNECT, _T("연결 중..."));
    SetLoginError(_T(""));

    // ── 별도 스레드에서 Connect 시도 (UI 블로킹 방지) ─────────
    std::string ip   = CT2A(strIP,   CP_UTF8);
    HWND hWnd        = GetSafeHwnd();

    std::thread([ip, port, hWnd]() {
        bool ok = NetworkManager::GetInstance().Connect(ip, port);
        ::PostMessage(hWnd, WM_CONNECT_RESULT, 0, ok ? 1 : 0);
    }).detach();
}

// ── 서버 연결 결과 (UI 스레드) ────────────────────────────────
LRESULT LoginDlg::OnConnectResult(WPARAM, LPARAM lParam)
{
    m_bConnecting.store(false);
    SetDlgItemText(IDC_BTN_CONNECT, _T("연결"));

    if (lParam == 1) {
        // 성공
        SetLoginError(_T(""));
        UpdateConnStatusUI();
        // 연결 성공 시 로그인 응답 콜백 재등록 (스레드 안전)
        NetworkManager::GetInstance().RegisterCallback(CmdCommon::REQ_LOGIN,
            [this](uint16_t, const std::string& body) {
                std::string* pBody = new std::string(body);
                this->PostMessage(WM_LOGIN_RESPONSE, 0, (LPARAM)pBody);
            });
    } else {
        // 실패
        UpdateConnStatusUI();
        CString strIP;
        this->GetDlgItemText(IDC_EDIT_SERVER_IP, strIP);
        CString err;
        err.Format(_T("서버 연결 실패 (%s)\n서버가 실행 중인지 확인하세요."), (LPCTSTR)strIP);
        SetLoginError(err);
        this->GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
    }
    return 0;
}

// ── 로그인 버튼 ───────────────────────────────────────────────
void LoginDlg::OnBnClickedOk()
{
    if (m_bWaiting.load()) return;

    CString strID, strPW;
    if (!ReadLoginFields(strID, strPW)) return;

    SetLoginError(_T(""));

    auto& net = NetworkManager::GetInstance();

    // ── 서버 미연결: 더미 로그인 (개발/테스트용) ─────────────
    if (!net.IsConnected()) {
        int ret = AfxMessageBox(
            _T("서버에 연결되지 않았습니다.\n")
            _T("테스트 모드로 로그인할까요?\n")
            _T("(실제 서버 연동 없이 진행됩니다)"),
            MB_YESNO | MB_ICONQUESTION);
        if (ret == IDYES) {
            std::string id = CT2A(strID, CP_UTF8);
            AuthManager::GetInstance().Login(id, "");
            CDialogEx::OnOK();
        }
        return;
    }

    // ── 서버 연결됨: 실제 로그인 요청 ────────────────────────
    m_bWaiting.store(true);
    this->GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("로그인 중..."));

    std::string id = CT2A(strID, CP_UTF8);
    std::string pw = CT2A(strPW, CP_UTF8);
    std::string json = "{\"id\":\"" + id + "\",\"pw\":\"" + pw + "\"}";

    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_LOGIN, json);
}

// ── 로그인 서버 응답 (UI 스레드) ──────────────────────────────
LRESULT LoginDlg::OnLoginResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);

    m_bWaiting.store(false);
    this->GetDlgItem(IDOK)->EnableWindow(TRUE);
    SetDlgItemText(IDOK, _T("로그인"));

    if (!pBody) return 0;

    int status = LJInt(*pBody, "status");
    delete pBody;

    if (status == (int)Status::SUCCESS) {
        // 성공: AuthManager에 세션 저장
        CString strID;
        this->GetDlgItemText(IDC_EDIT_LOGIN_ID, strID);
        std::string id = CT2A(strID, CP_UTF8);

        AuthManager::GetInstance().Login(id, "", m_strToken, m_strServerUserID);
        SetLoginError(_T(""));
        CDialogEx::OnOK();
    } else if (status == (int)Status::UNAUTHORIZED) {
        SetLoginError(_T("아이디 또는 비밀번호가 올바르지 않습니다."));
        this->GetDlgItem(IDC_EDIT_LOGIN_PW)->SetFocus();
        // 비밀번호 필드 초기화
        SetDlgItemText(IDC_EDIT_LOGIN_PW, _T(""));
    } else {
        SetLoginError(_T("로그인 중 오류가 발생했습니다. 잠시 후 다시 시도하세요."));
    }
    return 0;
}

// ── 회원가입 버튼 ─────────────────────────────────────────────
void LoginDlg::OnBnClickedGotoSignup()
{
    SignupDlg dlg(this);
    if (dlg.DoModal() == IDOK) {
        // 회원가입 성공 시 아이디 자동 입력
        SetDlgItemText(IDC_EDIT_LOGIN_ID, dlg.m_strResultID);
        SetDlgItemText(IDC_EDIT_LOGIN_PW, _T(""));
        SetLoginError(_T("회원가입 완료! 로그인해 주세요."));
        this->GetDlgItem(IDC_EDIT_LOGIN_PW)->SetFocus();
    }
}

void LoginDlg::OnBnClickedCancel()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCommon::REQ_LOGIN);
    CDialogEx::OnCancel();
}
