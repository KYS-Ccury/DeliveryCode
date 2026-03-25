// RegisterDlg.cpp - Registration with JSON protocol
// Step1: ID/PW/Phone, Step2: Address/Vehicle
#include "pch.h"
#include "RegisterDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(RegisterDlg, CDialogEx)

BEGIN_MESSAGE_MAP(RegisterDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_CHECK_ID, &RegisterDlg::OnBtnCheckId)
    ON_BN_CLICKED(IDC_BTN_NEXT,     &RegisterDlg::OnBtnNext)
    ON_BN_CLICKED(IDC_BTN_PREV,     &RegisterDlg::OnBtnPrev)
    ON_MESSAGE(WM_SOCKET_RECV,      &RegisterDlg::OnSocketRecv)
END_MESSAGE_MAP()

RegisterDlg::RegisterDlg(CWnd* pParent) : CDialogEx(IDD_REGISTER_DLG, pParent) {}
RegisterDlg::~RegisterDlg() {}

void RegisterDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_ID,         m_editId);
    DDX_Control(pDX, IDC_EDIT_PW,         m_editPw);
    DDX_Control(pDX, IDC_EDIT_PW_CONFIRM, m_editPwConfirm);
    DDX_Control(pDX, IDC_EDIT_PHONE,      m_editPhone);
    DDX_Control(pDX, IDC_EDIT_REGION,     m_editRegion);
    DDX_Control(pDX, IDC_CMB_VEHICLE,     m_cmbVehicle);
}

BOOL RegisterDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_cmbVehicle.AddString(_T("도보"));
    m_cmbVehicle.AddString(_T("자전거"));
    m_cmbVehicle.AddString(_T("오토바이"));
    m_cmbVehicle.AddString(_T("자동차"));
    m_cmbVehicle.SetCurSel(0);
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());
    ShowStep(1);
    return TRUE;
}

void RegisterDlg::ShowStep(int step)
{
    m_nStep = step;
    CString s; s.Format(_T("단계 %d / 2"), step);
    SetDlgItemText(IDC_STATIC_STEP, s);

    BOOL s1 = (step == 1), s2 = (step == 2);
    m_editId.ShowWindow(s1 ? SW_SHOW : SW_HIDE);
    m_editPw.ShowWindow(s1 ? SW_SHOW : SW_HIDE);
    m_editPwConfirm.ShowWindow(s1 ? SW_SHOW : SW_HIDE);
    m_editPhone.ShowWindow(s1 ? SW_SHOW : SW_HIDE);
    ShowDlgItem(IDC_BTN_CHECK_ID, s1);
    m_editRegion.ShowWindow(s2 ? SW_SHOW : SW_HIDE);
    m_cmbVehicle.ShowWindow(s2 ? SW_SHOW : SW_HIDE);
    ShowDlgItem(IDC_BTN_PREV, step > 1);
}

// Send: {"action":"CHECK_ID","login_id":"..."}
void RegisterDlg::OnBtnCheckId()
{
    CString strId; m_editId.GetWindowText(strId); strId.Trim();
    if (strId.GetLength() < 4) {
        MessageBox(_T("아이디는 4자 이상 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }

    CT2A idUtf8(strId, CP_UTF8);
    json req;
    req["action"] = "CHECK_ID";
    req["id"]     = std::string(idUtf8);   // 서버 handleSignup → "CHECK_ID" 분기

    bool bSent = AppContext::Get().socket.SendPacket(CMD_SIGNUP, req.dump());
    if (!bSent) {
        // 오프라인: 클라이언트에서 즉시 허용
        m_bIdChecked = true;
        MessageBox(_T("사용 가능한 아이디입니다."), _T("중복 확인"), MB_OK | MB_ICONINFORMATION);
    }
    // 서버 응답은 OnSocketRecv에서 처리
}

void RegisterDlg::OnBtnNext()
{
    if (m_nStep == 1) { if (!ValidateStep1()) return; ShowStep(2); }
    else if (m_nStep == 2) { if (!ValidateStep2()) return; DoRegister(); }
}

void RegisterDlg::OnBtnPrev()
{
    if (m_nStep > 1) ShowStep(m_nStep - 1);
}

bool RegisterDlg::ValidateStep1()
{
    CString strId, strPw, strPwConfirm, strPhone;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);
    m_editPwConfirm.GetWindowText(strPwConfirm);
    m_editPhone.GetWindowText(strPhone);
    strId.Trim(); strPw.Trim(); strPwConfirm.Trim(); strPhone.Trim();

    if (strId.GetLength() < 4) {
        MessageBox(_T("아이디는 4자 이상 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editId.SetFocus(); return false;
    }
    if (!m_bIdChecked) {
        MessageBox(_T("아이디 중복확인을 해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return false;
    }
    if (strPw.GetLength() < 10) {
        MessageBox(_T("비밀번호는 영문, 숫자 혼합 10자리 이상이어야 합니다."),
                   _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPw.SetFocus(); return false;
    }
    if (strPw != strPwConfirm) {
        MessageBox(_T("비밀번호가 일치하지 않습니다."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPwConfirm.SetFocus(); return false;
    }
    if (strPhone.IsEmpty()) {
        MessageBox(_T("휴대폰 번호를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editPhone.SetFocus(); return false;
    }
    return true;
}

bool RegisterDlg::ValidateStep2()
{
    CString strRegion; m_editRegion.GetWindowText(strRegion); strRegion.Trim();
    if (strRegion.IsEmpty()) {
        MessageBox(_T("거주지 주소를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editRegion.SetFocus(); return false;
    }
    if (m_cmbVehicle.GetCurSel() < 0) {
        MessageBox(_T("배달 수단을 선택하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
}

// Send: {"action":"REGISTER","login_id","password","phone","address","vehicle_type","role":"RIDER"}
void RegisterDlg::DoRegister()
{
    CString strId, strPw, strPhone, strRegion, strVehicle;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);
    m_editPhone.GetWindowText(strPhone);
    m_editRegion.GetWindowText(strRegion);
    int sel = m_cmbVehicle.GetCurSel();
    m_cmbVehicle.GetLBText(sel, strVehicle);

    AppContext::Get().session.loginId        = strId;
    AppContext::Get().session.deliveryRegion = strRegion;
    AppContext::Get().session.vehicleType    = strVehicle;

    auto toU = [](const CString& s) -> std::string {
        CT2A u(s, CP_UTF8); return std::string(u);
    };
    json req;
    // 서버 Basehandler::handleSignup 기대 키: id, pw, name, phone, address
    req["id"]           = toU(strId);
    req["pw"]           = toU(strPw);
    req["name"]         = toU(strId);       // 이름 미입력 시 ID로 대체
    req["phone"]        = toU(strPhone);
    req["address"]      = toU(strRegion);
    // vehicle_type은 서버 onSignup(RiderHandler)에서 rider_profiles INSERT 시 기본 BIKE 사용
    // → 클라이언트 선택값을 profile 변경으로 별도 전송
    req["vehicle_type"] = toU(strVehicle);  // onSignup에서 참조

    bool bSent = AppContext::Get().socket.SendPacket(CMD_SIGNUP, req.dump());
    if (!bSent) {
        MessageBox(_T("신규 가입이 완료되었습니다!\n로그인 후 배달을 시작하세요."),
                   _T("완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
}

// Recv: {"status":2000,"available":true} or {"status":2000} or {"status":4000,"message":"..."}
LRESULT RegisterDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    if (protocol != CMD_SIGNUP) return 0;

    try {
        json res = json::parse(body);
        int status = res.value("status", 0);

        if (res.contains("available")) {
            if (res["available"].get<bool>()) {
                m_bIdChecked = true;
                MessageBox(_T("사용 가능한 아이디입니다."), _T("중복 확인"), MB_OK | MB_ICONINFORMATION);
            } else {
                m_bIdChecked = false;
                MessageBox(_T("이미 사용 중인 아이디입니다."), _T("중복 확인"), MB_OK | MB_ICONWARNING);
                m_editId.SetFocus();
            }
            return 0;
        }

        if (status == STATUS_SUCCESS) {
            // 회원가입 완료 후 차량 종류 업데이트 (REQ_GET_PROFILE 104)
            // → 서버 onGetProfile에서 vehicle_type 업데이트 처리
            if (!AppContext::Get().session.vehicleType.IsEmpty()) {
                CT2A vtUtf8(AppContext::Get().session.vehicleType, CP_UTF8);
                json vtReq;
                vtReq["vehicle_type"] = std::string(vtUtf8);
                AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, vtReq.dump());
            }
            MessageBox(_T("신규 가입이 완료되었습니다!\n로그인 후 배달을 시작하세요."),
                       _T("완료"), MB_OK | MB_ICONINFORMATION);
            EndDialog(IDOK);
        } else {
            std::string msg = res.value("message", "Registration failed.");
            CA2T wMsg(msg.c_str(), CP_UTF8);
            MessageBox(CString(wMsg), _T("실패"), MB_OK | MB_ICONWARNING);
        }
    } catch (...) {}
    return 0;
}

void RegisterDlg::ShowDlgItem(UINT nID, BOOL bShow)
{
    CWnd* pWnd = GetDlgItem(nID);
    if (pWnd) pWnd->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}

HBRUSH RegisterDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
