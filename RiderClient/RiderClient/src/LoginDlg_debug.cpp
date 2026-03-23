// LoginDlg.cpp - JSON binary protocol login
// Server: 10.10.10.122:8080
#include "pch.h"
#include "LoginDlg.h"
#include "Protocol.h"
#include "RegisterDlg.h"
#include "PrepareDeliveryDlg.h"
#include "MainDlg.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_LOGIN,     &LoginDlg::OnBtnLogin)
    ON_BN_CLICKED(IDC_BTN_REGISTER,  &LoginDlg::OnBtnRegister)
    ON_BN_CLICKED(IDC_BTN_FIND_ID,   &LoginDlg::OnBtnFindId)
    ON_BN_CLICKED(IDC_BTN_FIND_PW,   &LoginDlg::OnBtnFindPw)
    ON_MESSAGE(WM_SOCKET_RECV,       &LoginDlg::OnSocketRecv)
END_MESSAGE_MAP()

LoginDlg::LoginDlg(CWnd* pParent) : CDialogEx(IDD_LOGIN_DLG, pParent) {}
LoginDlg::~LoginDlg() {}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_ID,       m_editId);
    DDX_Control(pDX, IDC_EDIT_PW,       m_editPw);
    DDX_Control(pDX, IDC_CHECK_SAVEID,  m_checkSaveId);
    DDX_Control(pDX, IDC_BTN_LOGIN,    m_btnLogin);
    DDX_Control(pDX, IDC_BTN_REGISTER, m_btnRegister);
}

BOOL LoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // Connect to server (10.10.10.122:8080)
    // Show result in title bar to avoid IDC_STATIC_CONN_STATUS dependency
    bool bConnected = AppContext::Get().socket.Connect(_T("10.10.10.122"), 8080);
    CString title;
    title.Format(_T("BaeminRider Login - %s"),
                 bConnected ? _T("서버 연결됨") : _T("서버 연결 실패 (오프라인)"));
    SetWindowText(title);

    LoadSavedId();
    m_editPw.SetFocus();
    return FALSE;
}

void LoginDlg::OnBtnLogin()
{
    CString strId, strPw;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);
    strId.Trim(); strPw.Trim();

    if (strId.IsEmpty()) {
        MessageBox(_T("아이디를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editId.SetFocus(); return;
    }
    if (strPw.IsEmpty()) {
        MessageBox(_T("비밀번호를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPw.SetFocus(); return;
    }

    if (m_checkSaveId.GetCheck() == BST_CHECKED) SaveId(strId);
    else SaveId(_T(""));

    CT2A idUtf8(strId, CP_UTF8);
    CT2A pwUtf8(strPw, CP_UTF8);
    json req;
    req["login_id"] = std::string(idUtf8);
    req["password"] = std::string(pwUtf8);

    bool bSent = AppContext::Get().socket.SendPacket(CMD_LOGIN, req.dump());
    if (!bSent) {
        // Offline mode
        AppContext::Get().session.loginId    = strId;
        AppContext::Get().session.name       = strId;
        AppContext::Get().session.riderId    = 1;
        AppContext::Get().session.isLoggedIn = true;
        OpenMainDlg();
    }
}

void LoginDlg::OnBtnRegister()
{
    RegisterDlg dlg(this);
    if (dlg.DoModal() == IDOK) {
        PrepareDeliveryDlg prepDlg(this);
        prepDlg.DoModal();
    }
}

void LoginDlg::OnBtnFindId()
{
    MessageBox(_T("가입 시 등록한 휴대폰 번호로 확인할 수 있습니다.\n고객센터: 1588-0000"), _T("아이디 찾기"), MB_OK | MB_ICONINFORMATION);
}
void LoginDlg::OnBtnFindPw()
{
    MessageBox(_T("가입 시 등록한 휴대폰 번호로 확인할 수 있습니다.\n고객센터: 1588-0000"), _T("비밀번호 찾기"), MB_OK | MB_ICONINFORMATION);
}

// Recv: {"status":2000,"rider_id":1,"name":"...","vehicle_type":"...","is_working":false}
LRESULT LoginDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    // [디버그] 어떤 protocol 번호로 응답이 오는지 확인
    CString dbg;
    dbg.Format(_T("[OnSocketRecv] protocol=%u  body=%s"),
               (unsigned)protocol,
               CString(CA2T(body.c_str(), CP_UTF8)));
    AfxMessageBox(dbg);

    if (protocol != CMD_LOGIN) return 0;

    try {
        json res    = json::parse(body);
        int  status = res.value("status", 0);

        if (status == STATUS_SUCCESS) {
            RiderSession& sess = AppContext::Get().session;
            sess.riderId    = res.value("rider_id", 0);
            sess.isLoggedIn = true;
            sess.isOnline   = res.value("is_working", false);

            auto toCS = [](const std::string& s) -> CString {
                CA2T ws(s.c_str(), CP_UTF8); return CString(ws);
            };
            sess.name        = toCS(res.value("name",         ""));
            sess.phone       = toCS(res.value("phone",        ""));
            sess.vehicleType = toCS(res.value("vehicle_type", ""));

            CString strId;
            m_editId.GetWindowText(strId);
            sess.loginId = strId;
            OpenMainDlg();
        } else {
            std::string msg = res.value("message", "Login failed.");
            CA2T wMsg(msg.c_str(), CP_UTF8);
            MessageBox(CString(wMsg), _T("로그인 실패"), MB_OK | MB_ICONWARNING);
            m_editPw.SetSel(0, -1);
            m_editPw.SetFocus();
        }
    } catch (...) {
        MessageBox(_T("서버 응답 오류"), _T("오류"), MB_OK | MB_ICONERROR);
    }
    return 0;
}

void LoginDlg::LoadSavedId()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\BaeminRider"),
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        TCHAR buf[256] = {};
        DWORD size = sizeof(buf), type = REG_SZ;
        if (RegQueryValueEx(hKey, _T("SavedId"), nullptr, &type,
                            reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS && buf[0])
        {
            m_editId.SetWindowText(buf);
            m_checkSaveId.SetCheck(BST_CHECKED);
        }
        RegCloseKey(hKey);
    }
}

void LoginDlg::SaveId(const CString& strId)
{
    HKEY hKey = nullptr;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\BaeminRider"),
                   0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (!hKey) return;
    if (strId.IsEmpty())
        RegDeleteValue(hKey, _T("SavedId"));
    else
        RegSetValueEx(hKey, _T("SavedId"), 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(static_cast<LPCTSTR>(strId)),
                      (strId.GetLength() + 1) * sizeof(TCHAR));
    RegCloseKey(hKey);
}

void LoginDlg::OpenMainDlg()
{
    ShowWindow(SW_HIDE);
    MainDlg mainDlg(GetParent());
    mainDlg.DoModal();
    AppContext::Get().session.Clear();
    AppContext::Get().currentOrder.Clear();
    m_editPw.SetWindowText(_T(""));
    ShowWindow(SW_SHOW);
    m_editPw.SetFocus();
}

HBRUSH LoginDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg)
            m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    // Buttons: do NOT override - let Windows draw button text normally
    return hbr;
}
