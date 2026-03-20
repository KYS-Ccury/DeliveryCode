#include "pch.h"
#include "PrepareDeliveryDlg.h"
#include "Protocol.h"
#include "AppContext.h"

IMPLEMENT_DYNAMIC(PrepareDeliveryDlg, CDialogEx)

BEGIN_MESSAGE_MAP(PrepareDeliveryDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_NEXT,    &PrepareDeliveryDlg::OnBtnNext)
    ON_BN_CLICKED(IDC_BTN_PREV,    &PrepareDeliveryDlg::OnBtnPrev)
    ON_MESSAGE(WM_SOCKET_RECV,     &PrepareDeliveryDlg::OnSocketRecv)
    ON_WM_TIMER()
END_MESSAGE_MAP()

PrepareDeliveryDlg::PrepareDeliveryDlg(CWnd* pParent)
    : CDialogEx(IDD_PREPARE_DLG, pParent)
{
}

PrepareDeliveryDlg::~PrepareDeliveryDlg()
{
}

void PrepareDeliveryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_RESIDENT_FRONT,  m_editResidentFront);
    DDX_Control(pDX, IDC_EDIT_RESIDENT_BACK,   m_editResidentBack);
    DDX_Control(pDX, IDC_EDIT_BANK,            m_editBank);
    DDX_Control(pDX, IDC_EDIT_ACCOUNT_HOLDER,  m_editAccountHolder);
    DDX_Control(pDX, IDC_EDIT_ACCOUNT_NUMBER,  m_editAccountNumber);
}

BOOL PrepareDeliveryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // 주민등록번호 입력 제한
    m_editResidentFront.SetLimitText(6);
    m_editResidentBack.SetLimitText(7);

    ShowStep(1);
    return TRUE;
}

// ─────────────────────────────────────────────
// 단계별 컨트롤 표시/숨김
// Step 1: 주민등록번호
// Step 2: 계좌정보
// Step 3: 제출 완료 메시지
// ─────────────────────────────────────────────
void PrepareDeliveryDlg::ShowStep(int step)
{
    m_nStep = step;

    // 스텝 레이블
    CString stepText;
    if (step <= 2)
        stepText.Format(_T("단계 %d / 2"), step);
    else
        stepText = _T("완료");
    SetDlgItemText(IDC_STATIC_STEP, stepText);

    BOOL showStep1 = (step == 1) ? TRUE : FALSE;
    BOOL showStep2 = (step == 2) ? TRUE : FALSE;
    BOOL showStep3 = (step == 3) ? TRUE : FALSE;

    // Step1 컨트롤
    m_editResidentFront.ShowWindow(showStep1 ? SW_SHOW : SW_HIDE);
    m_editResidentBack.ShowWindow(showStep1  ? SW_SHOW : SW_HIDE);

    // Step2 컨트롤
    m_editBank.ShowWindow(showStep2          ? SW_SHOW : SW_HIDE);
    m_editAccountHolder.ShowWindow(showStep2 ? SW_SHOW : SW_HIDE);
    m_editAccountNumber.ShowWindow(showStep2 ? SW_SHOW : SW_HIDE);

    // Step3 확인 메시지 Static
    CWnd* pConfirm = GetDlgItem(IDC_STATIC_CONFIRM_MSG);
    if (pConfirm) pConfirm->ShowWindow(showStep3 ? SW_SHOW : SW_HIDE);

    // 이전 버튼: step1에서 숨김
    CWnd* pPrev = GetDlgItem(IDC_BTN_PREV);
    if (pPrev) pPrev->ShowWindow(step > 1 && step < 3 ? SW_SHOW : SW_HIDE);

    // 다음/제출 버튼 텍스트
    CWnd* pNext = GetDlgItem(IDC_BTN_NEXT);
    if (pNext) {
        pNext->ShowWindow(step < 3 ? SW_SHOW : SW_HIDE);
        pNext->SetWindowText(step == 2 ? _T("제출하기") : _T("다음으로"));
    }

    if (step == 3) {
        // 확인 메시지 설정
        if (pConfirm)
            pConfirm->SetWindowText(_T("제출하신 정보를 확인 중이에요.\n잠시 후 자동으로 승인됩니다."));
    }
}

// ─────────────────────────────────────────────
// 다음/제출 버튼
// ─────────────────────────────────────────────
void PrepareDeliveryDlg::OnBtnNext()
{
    if (m_nStep == 1) {
        if (!ValidateStep1()) return;
        ShowStep(2);
    } else if (m_nStep == 2) {
        if (!ValidateStep2()) return;
        SubmitAll();
    }
}

// ─────────────────────────────────────────────
// 이전 버튼
// ─────────────────────────────────────────────
void PrepareDeliveryDlg::OnBtnPrev()
{
    if (m_nStep > 1 && m_nStep < 3)
        ShowStep(m_nStep - 1);
}

// ─────────────────────────────────────────────
// Step1 유효성 검사 (주민번호)
// ─────────────────────────────────────────────
bool PrepareDeliveryDlg::ValidateStep1()
{
    CString front, back;
    m_editResidentFront.GetWindowText(front);
    m_editResidentBack.GetWindowText(back);
    front.Trim(); back.Trim();

    if (front.GetLength() != 6 || back.GetLength() != 7) {
        MessageBox(_T("주민등록번호를 정확히 입력해주세요.\n(앞 6자리 - 뒤 7자리)"),
                   _T("알림"), MB_OK | MB_ICONWARNING);
        m_editResidentFront.SetFocus();
        return false;
    }
    // 숫자 여부 확인
    for (int i = 0; i < front.GetLength(); i++) {
        if (!_istdigit(front[i])) {
            MessageBox(_T("주민등록번호는 숫자만 입력 가능합니다."), _T("알림"), MB_OK | MB_ICONWARNING);
            return false;
        }
    }
    return true;
}

// ─────────────────────────────────────────────
// Step2 유효성 검사 (계좌)
// ─────────────────────────────────────────────
bool PrepareDeliveryDlg::ValidateStep2()
{
    CString bank, holder, account;
    m_editBank.GetWindowText(bank);
    m_editAccountHolder.GetWindowText(holder);
    m_editAccountNumber.GetWindowText(account);
    bank.Trim(); holder.Trim(); account.Trim();

    if (bank.IsEmpty()) {
        MessageBox(_T("은행명을 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editBank.SetFocus();
        return false;
    }
    if (holder.IsEmpty()) {
        MessageBox(_T("예금주를 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editAccountHolder.SetFocus();
        return false;
    }
    if (account.GetLength() < 10) {
        MessageBox(_T("계좌번호를 정확히 입력하세요."), _T("알림"), MB_OK | MB_ICONWARNING);
        m_editAccountNumber.SetFocus();
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
// 정보 제출 (서버 전송 → Step3 자동 승인)
// ─────────────────────────────────────────────
void PrepareDeliveryDlg::SubmitAll()
{
    CString bank, holder, account;
    m_editBank.GetWindowText(bank);
    m_editAccountHolder.GetWindowText(holder);
    m_editAccountNumber.GetWindowText(account);

    // 세션에 저장
    AppContext::Get().session.bankName       = bank;
    AppContext::Get().session.accountHolder  = holder;
    AppContext::Get().session.accountNumber  = account;

    // 서버로 전송 시도
    CString payload;
    payload.Format(_T("ACCT|%s\t%s\t%s"), bank, holder, account);
    AppContext::Get().socket.SendPacket(CMD_GET_MY_INFO, payload);

    // Step3 표시 후 3초 뒤 자동 승인
    ShowStep(3);
    SetTimer(1, 3000, nullptr);
}

// ─────────────────────────────────────────────
// 타이머: 자동 승인 완료
// ─────────────────────────────────────────────
void PrepareDeliveryDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        KillTimer(1);

        // 확인 메시지 변경
        CWnd* pConfirm = GetDlgItem(IDC_STATIC_CONFIRM_MSG);
        if (pConfirm)
            pConfirm->SetWindowText(_T("인증이 완료되었습니다!\n배달을 시작할 수 있습니다."));

        MessageBox(_T("인증이 완료되었습니다!\n이제 배달을 시작해보세요."),
                   _T("가입 완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
    CDialogEx::OnTimer(nIDEvent);
}

// ─────────────────────────────────────────────
// 서버 수신 처리
// ─────────────────────────────────────────────
LRESULT PrepareDeliveryDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;
    // 현재 PrepareDelivery에서는 서버 응답 별도 처리 없음
    // (자동 승인 타이머로 처리)
    return 0;
}
HBRUSH PrepareDeliveryDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    pDC->SetBkColor(RGB(225, 248, 242));
    pDC->SetTextColor(RGB(30, 60, 50));
    return m_hBrushBg;
}
