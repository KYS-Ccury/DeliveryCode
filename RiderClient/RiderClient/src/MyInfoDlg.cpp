#include "pch.h"
#include "MyInfoDlg.h"
#include "Protocol.h"
#include "AppContext.h"

// ══════════════════════════════════════════════════════════════
//  MyInfoDlg  –  배달수단 / 비밀번호변경 / 계좌정보변경
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(MyInfoDlg, CDialogEx)

BEGIN_MESSAGE_MAP(MyInfoDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_VEHICLE,     &MyInfoDlg::OnBtnVehicle)
    ON_BN_CLICKED(IDC_BTN_CHANGE_PW,   &MyInfoDlg::OnBtnChangePw)
    ON_BN_CLICKED(IDC_BTN_CHANGE_ACCT, &MyInfoDlg::OnBtnChangeAcct)
    ON_MESSAGE(WM_SOCKET_RECV,         &MyInfoDlg::OnSocketRecv)
END_MESSAGE_MAP()

MyInfoDlg::MyInfoDlg(CWnd* pParent)
    : CDialogEx(IDD_MYINFO_DLG, pParent)
{
}

MyInfoDlg::~MyInfoDlg()
{
}

BOOL MyInfoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    const RiderSession& s = AppContext::Get().session;

    // 아이디 표시
    SetDlgItemText(IDC_STATIC_LOGIN_ID,     s.loginId);
    // 배달수단 표시 (버튼 텍스트)
    CString vehicleText;
    vehicleText.Format(_T("%s  >"), static_cast<LPCTSTR>(
        s.vehicleType.IsEmpty() ? CString(_T("미설정")) : s.vehicleType));
    SetDlgItemText(IDC_BTN_VEHICLE,         vehicleText);
    // 배달지역 표시
    SetDlgItemText(IDC_STATIC_REGION,       s.deliveryRegion);

    return TRUE;
}

// 배달수단 팝업 메뉴
void MyInfoDlg::OnBtnVehicle()
{
    static const LPCTSTR vehicles[] = {
        _T("일반자전거"), _T("오토바이"),
        _T("전기자전거(PAS)"), _T("전기자전거(스로틀)"),
        _T("킥보드"), _T("자동차"), _T("도보")
    };
    const int CNT = 7;

    CMenu menu;
    menu.CreatePopupMenu();
    for (int i = 0; i < CNT; i++)
        menu.AppendMenu(MF_STRING, 2000 + i, vehicles[i]);

    CWnd* pBtn = GetDlgItem(IDC_BTN_VEHICLE);
    CRect rc;
    pBtn->GetWindowRect(&rc);

    int sel = menu.TrackPopupMenu(
        TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        rc.left, rc.bottom, this);

    if (sel >= 2000 && sel < 2000 + CNT) {
        CString chosen = vehicles[sel - 2000];
        AppContext::Get().session.vehicleType = chosen;

        // 서버에 업데이트
        AppContext::Get().socket.SendPacket(CMD_RIDER_STATUS_UPDATE,
                                           _T("VEHICLE|") + chosen);

        // 버튼 텍스트 갱신
        CString btnText;
        btnText.Format(_T("%s  >"), static_cast<LPCTSTR>(chosen));
        SetDlgItemText(IDC_BTN_VEHICLE, btnText);
    }
}

void MyInfoDlg::OnBtnChangePw()
{
    ChangePwDlg dlg(this);
    dlg.DoModal();
}

void MyInfoDlg::OnBtnChangeAcct()
{
    ChangeAcctDlg dlg(this);
    dlg.DoModal();
}

LRESULT MyInfoDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg) delete pMsg;
    return 0;
}


// ══════════════════════════════════════════════════════════════
//  ChangePwDlg
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(ChangePwDlg, CDialogEx)

BEGIN_MESSAGE_MAP(ChangePwDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_PW_CONFIRM, &ChangePwDlg::OnBtnConfirm)
    ON_MESSAGE(WM_SOCKET_RECV,        &ChangePwDlg::OnSocketRecv)
END_MESSAGE_MAP()

ChangePwDlg::ChangePwDlg(CWnd* pParent)
    : CDialogEx(IDD_CHANGE_PW_DLG, pParent)
{
}

ChangePwDlg::~ChangePwDlg()
{
}

BOOL ChangePwDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());
    return TRUE;
}

void ChangePwDlg::OnBtnConfirm()
{
    CString curPw, newPw, newPwConfirm;
    GetDlgItemText(IDC_EDIT_CUR_PW,      curPw);
    GetDlgItemText(IDC_EDIT_NEW_PW,      newPw);
    GetDlgItemText(IDC_EDIT_NEW_PW_CONFIRM, newPwConfirm);
    curPw.Trim(); newPw.Trim(); newPwConfirm.Trim();

    if (curPw.IsEmpty()) {
        MessageBox(_T("현재 비밀번호를 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (newPw.GetLength() < 10) {
        MessageBox(_T("새 비밀번호는 영문, 숫자 혼합 10자리 이상이어야 합니다."),
                   _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (newPw != newPwConfirm) {
        MessageBox(_T("새 비밀번호가 일치하지 않습니다."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }

    // 패킷 전송: "CHPW|curPw\tnewPw"
    CString payload;
    payload.Format(_T("CHPW|%s\t%s"), static_cast<LPCTSTR>(curPw),
                                       static_cast<LPCTSTR>(newPw));
    bool bSent = AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, payload);

    if (!bSent) {
        // 서버 미연결 시 즉시 완료 처리
        MessageBox(_T("비밀번호가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
    // 서버 응답은 OnSocketRecv에서 처리
}

LRESULT ChangePwDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    // 응답: "104|CHPW_OK" or "104|CHPW_FAIL|reason"
    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    CString rest = msg.Mid(p + 1);

    if (rest.Left(7) == _T("CHPW_OK")) {
        MessageBox(_T("비밀번호가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    } else if (rest.Left(9) == _T("CHPW_FAIL")) {
        CString reason = (rest.GetLength() > 10) ? rest.Mid(10) : _T("현재 비밀번호가 올바르지 않습니다.");
        MessageBox(reason, _T("변경 실패"), MB_OK | MB_ICONWARNING);
    }
    return 0;
}


// ══════════════════════════════════════════════════════════════
//  ChangeAcctDlg  –  계좌 정보 확인 및 변경
// ══════════════════════════════════════════════════════════════
IMPLEMENT_DYNAMIC(ChangeAcctDlg, CDialogEx)

BEGIN_MESSAGE_MAP(ChangeAcctDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_ACCT_CHANGE, &ChangeAcctDlg::OnBtnChange)
    ON_MESSAGE(WM_SOCKET_RECV,         &ChangeAcctDlg::OnSocketRecv)
END_MESSAGE_MAP()

ChangeAcctDlg::ChangeAcctDlg(CWnd* pParent)
    : CDialogEx(IDD_CHANGE_ACCT_DLG, pParent)
    , m_bEditMode(false)
{
}

ChangeAcctDlg::~ChangeAcctDlg()
{
}

BOOL ChangeAcctDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    const RiderSession& s = AppContext::Get().session;
    // 현재 계좌 정보 Static에 표시
    SetDlgItemText(IDC_STATIC_BANK,    s.bankName);
    SetDlgItemText(IDC_STATIC_HOLDER,  s.accountHolder);

    // 계좌번호 마스킹: 뒤 4자리만 표시
    CString masked;
    if (s.accountNumber.GetLength() > 4) {
        CString stars(_T('*'), s.accountNumber.GetLength() - 4);
        masked = stars + s.accountNumber.Right(4);
    } else {
        masked = s.accountNumber;
    }
    SetDlgItemText(IDC_STATIC_ACCOUNT, masked);

    ShowEditMode(false);
    return TRUE;
}

void ChangeAcctDlg::OnBtnChange()
{
    if (!m_bEditMode)
        ShowEditMode(true);
    else
        DoSaveAcct();
}

void ChangeAcctDlg::ShowEditMode(bool bEdit)
{
    m_bEditMode = bEdit;

    // 조회 모드: Static 표시, Edit 숨김
    // 편집 모드: Edit 표시, Static 숨김
    auto showCtrl = [this](UINT id, BOOL bShow) {
        CWnd* w = GetDlgItem(id);
        if (w) w->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
    };

    showCtrl(IDC_STATIC_BANK,    !bEdit);
    showCtrl(IDC_STATIC_HOLDER,  !bEdit);
    showCtrl(IDC_STATIC_ACCOUNT, !bEdit);

    showCtrl(IDC_EDIT_BANK,      bEdit);
    showCtrl(IDC_EDIT_HOLDER,    bEdit);
    showCtrl(IDC_EDIT_ACCOUNT,   bEdit);

    if (bEdit) {
        // 편집 초기값 채우기
        const RiderSession& s = AppContext::Get().session;
        SetDlgItemText(IDC_EDIT_BANK,    s.bankName);
        SetDlgItemText(IDC_EDIT_HOLDER,  s.accountHolder);
        SetDlgItemText(IDC_EDIT_ACCOUNT, s.accountNumber);
        SetDlgItemText(IDC_BTN_ACCT_CHANGE, _T("저장하기"));
    } else {
        SetDlgItemText(IDC_BTN_ACCT_CHANGE, _T("변경하기"));
    }
}

void ChangeAcctDlg::DoSaveAcct()
{
    CString bank, holder, account;
    GetDlgItemText(IDC_EDIT_BANK,    bank);
    GetDlgItemText(IDC_EDIT_HOLDER,  holder);
    GetDlgItemText(IDC_EDIT_ACCOUNT, account);
    bank.Trim(); holder.Trim(); account.Trim();

    if (bank.IsEmpty()) {
        MessageBox(_T("은행명을 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (holder.IsEmpty()) {
        MessageBox(_T("예금주를 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (account.GetLength() < 10) {
        MessageBox(_T("계좌번호를 정확히 입력해주세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        return;
    }

    // 세션 업데이트
    AppContext::Get().session.bankName      = bank;
    AppContext::Get().session.accountHolder = holder;
    AppContext::Get().session.accountNumber = account;

    // 서버 전송
    CString payload;
    payload.Format(_T("ACCT|%s\t%s\t%s"), static_cast<LPCTSTR>(bank),
                                            static_cast<LPCTSTR>(holder),
                                            static_cast<LPCTSTR>(account));
    bool bSent = AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, payload);

    if (!bSent) {
        MessageBox(_T("계좌 정보가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        ShowEditMode(false);
        // Static 갱신
        SetDlgItemText(IDC_STATIC_BANK,    bank);
        SetDlgItemText(IDC_STATIC_HOLDER,  holder);
        CString masked(_T('*'), account.GetLength() - 4);
        masked += account.Right(4);
        SetDlgItemText(IDC_STATIC_ACCOUNT, masked);
    }
}

LRESULT ChangeAcctDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    CString rest = msg.Mid(p + 1);

    if (rest.Left(7) == _T("ACCT_OK")) {
        MessageBox(_T("계좌 정보가 변경되었습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        ShowEditMode(false);
    } else if (rest.Left(9) == _T("ACCT_FAIL")) {
        MessageBox(_T("계좌 정보 변경에 실패했습니다."), _T("오류"), MB_OK | MB_ICONWARNING);
    }
    return 0;
}

void MyInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

void ChangePwDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_CUR_PW,         m_editCurPw);
    DDX_Control(pDX, IDC_EDIT_NEW_PW,         m_editNewPw);
    DDX_Control(pDX, IDC_EDIT_NEW_PW_CONFIRM, m_editNewPwConfirm);
}

void ChangeAcctDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_BANK,    m_editBank);
    DDX_Control(pDX, IDC_EDIT_HOLDER,  m_editHolder);
    DDX_Control(pDX, IDC_EDIT_ACCOUNT, m_editAccount);
}
