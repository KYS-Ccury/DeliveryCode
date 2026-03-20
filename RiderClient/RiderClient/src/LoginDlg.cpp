#include "pch.h"
#include "LoginDlg.h"
#include "Protocol.h"
#include "RegisterDlg.h"
#include "PrepareDeliveryDlg.h"
#include "MainDlg.h"
#include "AppContext.h"

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_LOGIN,     &LoginDlg::OnBtnLogin)
    ON_BN_CLICKED(IDC_BTN_REGISTER,  &LoginDlg::OnBtnRegister)
    ON_BN_CLICKED(IDC_BTN_FIND_ID,   &LoginDlg::OnBtnFindId)
    ON_BN_CLICKED(IDC_BTN_FIND_PW,   &LoginDlg::OnBtnFindPw)
    ON_MESSAGE(WM_SOCKET_RECV,       &LoginDlg::OnSocketRecv)
END_MESSAGE_MAP()

LoginDlg::LoginDlg(CWnd* pParent)
    : CDialogEx(IDD_LOGIN_DLG, pParent)
{
}

LoginDlg::~LoginDlg()
{
}

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

    // 소켓 알림 윈도우 설정
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // 서버 연결 시도 (서버 없이도 UI 동작하도록 실패해도 무시)
    AppContext::Get().socket.Connect(_T("127.0.0.1"), 9000);

    // 저장된 아이디 불러오기
    LoadSavedId();

    // PW 에디트에 포커스
    m_editPw.SetFocus();

    return FALSE; // FALSE = SetFocus를 직접 처리했으므로
}

// ─────────────────────────────────────────────
// 로그인 버튼
// ─────────────────────────────────────────────
void LoginDlg::OnBtnLogin()
{
    CString strId, strPw;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);

    strId.Trim();
    strPw.Trim();

    if (strId.IsEmpty()) {
        MessageBox(_T("아이디를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editId.SetFocus();
        return;
    }
    if (strPw.IsEmpty()) {
        MessageBox(_T("비밀번호를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPw.SetFocus();
        return;
    }

    // 아이디 저장 처리
    if (m_checkSaveId.GetCheck() == BST_CHECKED)
        SaveId(strId);
    else
        SaveId(_T("")); // 저장 해제

    // 패킷 전송: "101|id\tpw"
    CString payload;
    payload.Format(_T("%s\t%s"), strId, strPw);
    bool bSent = AppContext::Get().socket.SendPacket(CMD_LOGIN, payload);

    if (!bSent) {
        // 서버 미연결 상태 → 임시로 직접 메인으로 전환 (UI 테스트용)
        AppContext::Get().session.loginId   = strId;
        AppContext::Get().session.name      = strId;
        AppContext::Get().session.riderId   = 1;
        AppContext::Get().session.isLoggedIn = true;
        OpenMainDlg();
    }
    // 서버 연결 상태에서는 OnSocketRecv에서 응답 처리
}

// ─────────────────────────────────────────────
// 회원가입 버튼
// ─────────────────────────────────────────────
void LoginDlg::OnBtnRegister()
{
    RegisterDlg dlg(this);
    if (dlg.DoModal() == IDOK) {
        // 회원가입 완료 후 PrepareDeliveryDlg (주민번호/계좌) 표시
        PrepareDeliveryDlg prepDlg(this);
        prepDlg.DoModal();
    }
}

// ─────────────────────────────────────────────
// 아이디 찾기 (형태만 구현)
// ─────────────────────────────────────────────
void LoginDlg::OnBtnFindId()
{
    MessageBox(
        _T("가입 시 등록한 휴대폰 번호로 아이디를 확인할 수 있습니다.\n"
           "고객센터 : 1588-0000"),
        _T("아이디 찾기"), MB_OK | MB_ICONINFORMATION);
}

// ─────────────────────────────────────────────
// 비밀번호 찾기 (형태만 구현)
// ─────────────────────────────────────────────
void LoginDlg::OnBtnFindPw()
{
    MessageBox(
        _T("아이디와 등록 휴대폰 번호를 통해 비밀번호를 재설정할 수 있습니다.\n"
           "고객센터 : 1588-0000"),
        _T("비밀번호 찾기"), MB_OK | MB_ICONINFORMATION);
}

// ─────────────────────────────────────────────
// 서버 수신 처리
// 패킷 형식: "101|OK|riderId|name|region|vehicle"
//           "101|FAIL|reason"
// ─────────────────────────────────────────────
LRESULT LoginDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    // CMD 확인
    int pipePos = msg.Find(_T('|'));
    if (pipePos < 0) return 0;

    int cmd = _ttoi(msg.Left(pipePos));
    if (cmd != CMD_LOGIN) return 0;

    CString rest = msg.Mid(pipePos + 1);

    // 결과 파싱: OK|riderId|name|region|vehicle
    int p2 = rest.Find(_T('|'));
    CString result = (p2 >= 0) ? rest.Left(p2) : rest;

    if (result == _T("OK")) {
        // 세션 채우기
        CString fields = rest.Mid(p2 + 1); // "riderId|name|region|vehicle"
        int f1 = fields.Find(_T('|'));
        AppContext::Get().session.riderId   = _ttoi(fields.Left(f1));
        fields = fields.Mid(f1 + 1);

        int f2 = fields.Find(_T('|'));
        AppContext::Get().session.name      = fields.Left(f2);
        fields = fields.Mid(f2 + 1);

        int f3 = fields.Find(_T('|'));
        AppContext::Get().session.deliveryRegion = fields.Left(f3 >= 0 ? f3 : fields.GetLength());
        if (f3 >= 0) AppContext::Get().session.vehicleType = fields.Mid(f3 + 1);

        CString strId;
        m_editId.GetWindowText(strId);
        AppContext::Get().session.loginId   = strId;
        AppContext::Get().session.isLoggedIn = true;

        OpenMainDlg();
    } else {
        // 로그인 실패
        CString reason = (p2 >= 0 && rest.GetLength() > p2 + 1) ? rest.Mid(p2 + 1) : _T("아이디 또는 비밀번호가 올바르지 않습니다.");
        MessageBox(reason, _T("로그인 실패"), MB_OK | MB_ICONWARNING);
        m_editPw.SetSel(0, -1);
        m_editPw.SetFocus();
    }

    return 0;
}

// ─────────────────────────────────────────────
// 저장된 아이디 불러오기 (레지스트리)
// ─────────────────────────────────────────────
void LoginDlg::LoadSavedId()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("Software\\BaeminRider"),
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR buf[256] = {};
        DWORD size = sizeof(buf);
        DWORD type = REG_SZ;
        if (RegQueryValueEx(hKey, _T("SavedId"), nullptr, &type,
                            reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS
            && buf[0] != _T('\0'))
        {
            m_editId.SetWindowText(buf);
            m_checkSaveId.SetCheck(BST_CHECKED);
        }
        RegCloseKey(hKey);
    }
}

// ─────────────────────────────────────────────
// 아이디 저장 / 삭제 (레지스트리)
// ─────────────────────────────────────────────
void LoginDlg::SaveId(const CString& strId)
{
    HKEY hKey = nullptr;
    RegCreateKeyEx(HKEY_CURRENT_USER,
                   _T("Software\\BaeminRider"),
                   0, nullptr, REG_OPTION_NON_VOLATILE,
                   KEY_WRITE, nullptr, &hKey, nullptr);
    if (!hKey) return;

    if (strId.IsEmpty()) {
        RegDeleteValue(hKey, _T("SavedId"));
    } else {
        RegSetValueEx(hKey, _T("SavedId"), 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(static_cast<LPCTSTR>(strId)),
                      (strId.GetLength() + 1) * sizeof(TCHAR));
    }
    RegCloseKey(hKey);
}

// ─────────────────────────────────────────────
// 메인 다이얼로그로 전환
// ─────────────────────────────────────────────
void LoginDlg::OpenMainDlg()
{
    // 현재 창 숨기기
    ShowWindow(SW_HIDE);

    MainDlg mainDlg(GetParent());
    mainDlg.DoModal();

    // MainDlg 종료 후 로그인 창 다시 표시 (로그아웃 처리)
    AppContext::Get().session.Clear();
    AppContext::Get().currentOrder.Clear();
    m_editPw.SetWindowText(_T(""));
    ShowWindow(SW_SHOW);
    m_editPw.SetFocus();
}
HBRUSH LoginDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    pDC->SetBkColor(RGB(225, 248, 242));
    pDC->SetTextColor(RGB(30, 60, 50));
    return m_hBrushBg;
}
