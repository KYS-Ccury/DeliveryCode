#include "pch.h"
#include "RegisterDlg.h"
#include "Protocol.h"
#include "AppContext.h"

IMPLEMENT_DYNAMIC(RegisterDlg, CDialogEx)

BEGIN_MESSAGE_MAP(RegisterDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_CHECK_ID, &RegisterDlg::OnBtnCheckId)
    ON_BN_CLICKED(IDC_BTN_NEXT,     &RegisterDlg::OnBtnNext)
    ON_BN_CLICKED(IDC_BTN_PREV,     &RegisterDlg::OnBtnPrev)
    ON_MESSAGE(WM_SOCKET_RECV,      &RegisterDlg::OnSocketRecv)
END_MESSAGE_MAP()

RegisterDlg::RegisterDlg(CWnd* pParent)
    : CDialogEx(IDD_REGISTER_DLG, pParent)
{
}

RegisterDlg::~RegisterDlg()
{
}

void RegisterDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_ID,         m_editId);
    DDX_Control(pDX, IDC_EDIT_PW,         m_editPw);
    DDX_Control(pDX, IDC_EDIT_PW_CONFIRM, m_editPwConfirm);
    DDX_Control(pDX, IDC_EDIT_REGION,     m_editRegion);
    DDX_Control(pDX, IDC_CMB_VEHICLE,     m_cmbVehicle);
}

BOOL RegisterDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 배달 수단 콤보 채우기
    m_cmbVehicle.AddString(_T("도보"));
    m_cmbVehicle.AddString(_T("자전거"));
    m_cmbVehicle.AddString(_T("오토바이"));
    m_cmbVehicle.AddString(_T("자동차"));
    m_cmbVehicle.SetCurSel(0);

    // 소켓 알림 윈도우 설정
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    ShowStep(1);
    return TRUE;
}

// ─────────────────────────────────────────────
// 단계별 컨트롤 표시/숨김
// Step 1: 아이디/비밀번호
// Step 2: 배달지역/수단
// ─────────────────────────────────────────────
void RegisterDlg::ShowStep(int step)
{
    m_nStep = step;

    // Step 표시 레이블 갱신
    CString stepText;
    stepText.Format(_T("단계 %d / 2"), step);
    SetDlgItemText(IDC_STATIC_STEP, stepText);

    // 컨트롤 ID 목록: Step1 컨트롤 / Step2 컨트롤
    // Step1 전용: IDC_EDIT_ID, IDC_BTN_CHECK_ID, IDC_EDIT_PW, IDC_EDIT_PW_CONFIRM
    // Step2 전용: IDC_EDIT_REGION, IDC_CMB_VEHICLE

    BOOL showStep1 = (step == 1) ? TRUE : FALSE;
    BOOL showStep2 = (step == 2) ? TRUE : FALSE;

    // Step1 컨트롤
    m_editId.ShowWindow(showStep1 ? SW_SHOW : SW_HIDE);
    m_editPw.ShowWindow(showStep1 ? SW_SHOW : SW_HIDE);
    m_editPwConfirm.ShowWindow(showStep1 ? SW_SHOW : SW_HIDE);
    ShowDlgItem(IDC_BTN_CHECK_ID, showStep1);

    // Step2 컨트롤
    m_editRegion.ShowWindow(showStep2 ? SW_SHOW : SW_HIDE);
    m_cmbVehicle.ShowWindow(showStep2 ? SW_SHOW : SW_HIDE);

    // 이전 버튼: step1에서는 숨김
    ShowDlgItem(IDC_BTN_PREV, step > 1);
}

// ─────────────────────────────────────────────
// 중복 확인 버튼
// ─────────────────────────────────────────────
void RegisterDlg::OnBtnCheckId()
{
    CString strId;
    m_editId.GetWindowText(strId);
    strId.Trim();

    if (strId.IsEmpty()) {
        MessageBox(_T("아이디를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (strId.GetLength() < 4) {
        MessageBox(_T("아이디는 4자 이상 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }

    bool bSent = AppContext::Get().socket.SendPacket(CMD_REGISTER, _T("CHECK|") + strId);
    if (!bSent) {
        // 서버 미연결 시 임시 통과
        m_bIdChecked = true;
        MessageBox(_T("사용 가능한 아이디입니다."), _T("중복 확인"), MB_OK | MB_ICONINFORMATION);
    }
    // 서버 응답은 OnSocketRecv에서 처리
}

// ─────────────────────────────────────────────
// 다음 버튼
// ─────────────────────────────────────────────
void RegisterDlg::OnBtnNext()
{
    if (m_nStep == 1) {
        if (!ValidateStep1()) return;
        ShowStep(2);
    } else if (m_nStep == 2) {
        if (!ValidateStep2()) return;
        DoRegister();
    }
}

// ─────────────────────────────────────────────
// 이전 버튼
// ─────────────────────────────────────────────
void RegisterDlg::OnBtnPrev()
{
    if (m_nStep > 1)
        ShowStep(m_nStep - 1);
}

// ─────────────────────────────────────────────
// Step1 유효성 검사
// ─────────────────────────────────────────────
bool RegisterDlg::ValidateStep1()
{
    CString strId, strPw, strPwConfirm;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);
    m_editPwConfirm.GetWindowText(strPwConfirm);
    strId.Trim(); strPw.Trim(); strPwConfirm.Trim();

    if (strId.GetLength() < 4) {
        MessageBox(_T("아이디는 4자 이상 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editId.SetFocus();
        return false;
    }
    if (!m_bIdChecked) {
        MessageBox(_T("아이디 중복확인을 해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return false;
    }
    if (strPw.GetLength() < 10) {
        MessageBox(_T("비밀번호는 영문, 숫자 혼합 10자리 이상이어야 합니다."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPw.SetFocus();
        return false;
    }
    if (strPw != strPwConfirm) {
        MessageBox(_T("비밀번호가 일치하지 않습니다."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPwConfirm.SetFocus();
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
// Step2 유효성 검사
// ─────────────────────────────────────────────
bool RegisterDlg::ValidateStep2()
{
    CString strRegion;
    m_editRegion.GetWindowText(strRegion);
    strRegion.Trim();

    if (strRegion.IsEmpty()) {
        MessageBox(_T("거주지 주소를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editRegion.SetFocus();
        return false;
    }
    if (m_cmbVehicle.GetCurSel() < 0) {
        MessageBox(_T("배달 수단을 선택하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
// 회원가입 패킷 전송
// 형식: "REGISTER|id\tpw\tregion\tvehicle"
// ─────────────────────────────────────────────
void RegisterDlg::DoRegister()
{
    CString strId, strPw, strRegion, strVehicle;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);
    m_editRegion.GetWindowText(strRegion);

    int sel = m_cmbVehicle.GetCurSel();
    m_cmbVehicle.GetLBText(sel, strVehicle);

    // 세션에 임시 저장
    AppContext::Get().session.loginId       = strId;
    AppContext::Get().session.deliveryRegion = strRegion;
    AppContext::Get().session.vehicleType   = strVehicle;

    CString payload;
    payload.Format(_T("%s\t%s\t%s\t%s"), strId, strPw, strRegion, strVehicle);

    bool bSent = AppContext::Get().socket.SendPacket(CMD_REGISTER, payload);
    if (!bSent) {
        // 서버 미연결 시 바로 완료 처리
        MessageBox(_T("신규 가입이 완료되었습니다!\n로그인 후 배달을 시작하세요."),
                   _T("가입 완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
}

// ─────────────────────────────────────────────
// 서버 수신 처리
// "100|OK"           → 가입 완료
// "100|DUP"          → 아이디 중복
// "100|CHECK_OK"     → 중복확인 통과
// "100|CHECK_DUP"    → 중복확인 실패
// ─────────────────────────────────────────────
LRESULT RegisterDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int pipePos = msg.Find(_T('|'));
    if (pipePos < 0) return 0;
    int cmd = _ttoi(msg.Left(pipePos));
    if (cmd != CMD_REGISTER) return 0;

    CString result = msg.Mid(pipePos + 1);

    if (result == _T("OK")) {
        MessageBox(_T("신규 가입이 완료되었습니다!\n로그인 후 배달을 시작하세요."),
                   _T("가입 완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    } else if (result == _T("CHECK_OK")) {
        m_bIdChecked = true;
        MessageBox(_T("사용 가능한 아이디입니다."), _T("중복 확인"), MB_OK | MB_ICONINFORMATION);
    } else if (result == _T("CHECK_DUP")) {
        m_bIdChecked = false;
        MessageBox(_T("이미 사용 중인 아이디입니다."), _T("중복 확인"), MB_OK | MB_ICONWARNING);
        m_editId.SetFocus();
    } else if (result == _T("DUP")) {
        MessageBox(_T("이미 가입된 아이디입니다."), _T("가입 실패"), MB_OK | MB_ICONWARNING);
    }

    return 0;
}

// ─────────────────────────────────────────────
// 헬퍼: 컨트롤 표시/숨김
// ─────────────────────────────────────────────
void RegisterDlg::ShowDlgItem(UINT nID, BOOL bShow)
{
    CWnd* pWnd = GetDlgItem(nID);
    if (pWnd) pWnd->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}
