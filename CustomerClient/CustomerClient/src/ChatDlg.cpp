// ================================================================
//  ChatDlg.cpp  (고객 채팅 다이얼로그 완성 버전)
//
//  기존 ChatDlg.cpp 를 완전히 대체한다.
//
//  ▶ 흐름
//    OnInitDialog
//      → ChatManager::CreateOrGetRoom(targetType, orderId)
//         → 서버에 600 전송
//         → 응답 수신 시: joinRoom + RequestHistory(602 전송)
//      → RegisterReceiveCallback(hWnd) : HWND 등록
//
//    WM_CHAT_HISTORY (WM_USER+102)
//      → 과거 메시지 vector 수신 → 리스트박스 일괄 표시
//
//    WM_CHAT_RECEIVED (WM_USER+101)
//      → 실시간 ChatMessage* 수신 → 리스트박스 1건 추가
//
//    OnBnClickedBtnChatSend
//      → ChatManager::SendMessage(msg) → 601 전송
//      → 내 메시지는 로컬에서 바로 표시 (에코는 NTF_RECV_MSG 로도 옴)
//
//    OnBnClickedBtnChatBack
//      → UnregisterReceiveCallback → EndDialog
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ChatDlg.h"
#include "ChatManager.h"
#include "AuthManager.h"

IMPLEMENT_DYNAMIC(ChatDlg, CDialogEx)

ChatDlg::ChatDlg(CWnd* pParent)
    : CDialogEx(IDD_CHAT_DLG, pParent) {}
ChatDlg::~ChatDlg() {}

void ChatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ChatDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_CHAT_BACK, &ChatDlg::OnBnClickedBtnChatBack)
    ON_BN_CLICKED(IDC_BTN_CHAT_SEND, &ChatDlg::OnBnClickedBtnChatSend)
    ON_MESSAGE(WM_CHAT_RECEIVED,     &ChatDlg::OnChatReceived)
    ON_MESSAGE(WM_CHAT_HISTORY,      &ChatDlg::OnChatHistory)
END_MESSAGE_MAP()

// ================================================================
//  OnInitDialog
// ================================================================
BOOL ChatDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // ── 타이틀 설정 ──────────────────────────────────────────
    SetDlgItemText(IDC_STATIC_CHAT_TITLE,
        m_strTargetName.IsEmpty() ? _T("채팅") : m_strTargetName);

    auto& cm = ChatManager::GetInstance();

    // ── HWND 먼저 등록 (콜백이 hWnd 를 참조함) ───────────────
    cm.RegisterReceiveCallback(GetSafeHwnd());

    // ── 채팅방 생성/입장 요청 ─────────────────────────────────
    // target_type 결정
    std::string targetType = "admin"; // 기본값: 관리자 채팅
    if (!m_strTargetType.IsEmpty()) {
        std::string t = std::string(CT2A(m_strTargetType, CP_UTF8));
        if (t == "owner" || t == "store") targetType = "owner";
    }

    // CreateOrGetRoom → 서버에 600 전송
    // 내부에서 방 준비 완료 시 RegisterNtfCallback + RegisterHistoryCallback
    // + RequestHistory(602) 를 자동으로 수행함
    cm.CreateOrGetRoom(targetType, m_nOrderId);

    // ── "연결 중..." 안내 ─────────────────────────────────────
    CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (pList) pList->AddString(_T("── 채팅방에 연결 중입니다... ──"));

    return TRUE;
}

// ================================================================
//  ClearMessages  — 리스트박스 전체 초기화
// ================================================================
void ChatDlg::ClearMessages()
{
    CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (pList) pList->ResetContent();
}

// ================================================================
//  AppendMessage  — 리스트박스에 메시지 1건 추가
// ================================================================
void ChatDlg::AppendMessage(const ChatMessage& msg)
{
    CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (!pList) return;

    CString strMsg  = CA2T(msg.message.c_str(),   CP_UTF8);
    CString strTime = CA2T(msg.timestamp.c_str(), CP_UTF8);
    CString line;

    if (msg.isMine) {
        // 내 메시지: 오른쪽 정렬 느낌으로 구분
        if (strTime.IsEmpty())
            line.Format(_T("  [나] %s"),          (LPCTSTR)strMsg);
        else
            line.Format(_T("  [나] %s  (%s)"),    (LPCTSTR)strMsg, (LPCTSTR)strTime);
    } else {
        // 상대방 메시지
        CString strRole = CA2T(msg.senderRole.c_str(), CP_UTF8);
        if (strRole.IsEmpty()) strRole = _T("상대방");

        if (strTime.IsEmpty())
            line.Format(_T("[%s] %s"),           (LPCTSTR)strRole, (LPCTSTR)strMsg);
        else
            line.Format(_T("[%s] %s  (%s)"),     (LPCTSTR)strRole, (LPCTSTR)strMsg,
                                                 (LPCTSTR)strTime);
    }

    int idx = pList->AddString(line);
    pList->SetCurSel(idx);   // 맨 아래로 스크롤
}

// ================================================================
//  OnChatHistory  (WM_CHAT_HISTORY = WM_USER+102)
//  과거 메시지 vector* 수신 → 리스트박스 일괄 표시
// ================================================================
LRESULT ChatDlg::OnChatHistory(WPARAM /*wParam*/, LPARAM lParam)
{
    std::vector<ChatMessage>* pList =
        reinterpret_cast<std::vector<ChatMessage>*>(lParam);
    if (!pList) return 0;

    ClearMessages();

    if (pList->empty()) {
        CListBox* pLB = (CListBox*)GetDlgItem(IDC_LIST_CHAT_MSGS);
        if (pLB) pLB->AddString(_T("── 대화 내역이 없습니다. ──"));
    } else {
        for (const auto& msg : *pList)
            AppendMessage(msg);
    }

    delete pList;
    return 0;
}

// ================================================================
//  OnChatReceived  (WM_CHAT_RECEIVED = WM_USER+101)
//  실시간 메시지 ChatMessage* 수신
// ================================================================
LRESULT ChatDlg::OnChatReceived(WPARAM /*wParam*/, LPARAM lParam)
{
    ChatMessage* pMsg = reinterpret_cast<ChatMessage*>(lParam);
    if (pMsg) {
        // 서버 에코로 내 메시지가 다시 오면 중복 방지를 위해
        // isMine == true 인 경우는 이미 OnBnClickedBtnChatSend 에서 표시됨
        // → 여기서는 상대방 메시지만 표시
        if (!pMsg->isMine)
            AppendMessage(*pMsg);
        delete pMsg;
    }
    return 0;
}

// ================================================================
//  OnBnClickedBtnChatSend  — 전송 버튼
// ================================================================
void ChatDlg::OnBnClickedBtnChatSend()
{
    CString strInput;
    GetDlgItemText(IDC_EDIT_CHAT_INPUT, strInput);
    if (strInput.IsEmpty()) return;

    std::string msg = std::string(CT2A(strInput, CP_UTF8));

    auto& cm = ChatManager::GetInstance();

    // 방이 아직 준비 안됐으면 안내
    if (!cm.IsRoomReady()) {
        AfxMessageBox(_T("채팅방에 연결 중입니다. 잠시 후 다시 시도하세요."),
                      MB_ICONINFORMATION);
        return;
    }

    if (cm.SendMessage(msg)) {
        // 내 메시지 즉시 표시 (에코 중복 방지: OnChatReceived 에서는 isMine 스킵)
        ChatMessage mine;
        mine.senderRole = "CUSTOMER";
        mine.senderID   = AuthManager::GetInstance().GetCurrentUserID();
        mine.message    = msg;
        mine.isMine     = true;
        AppendMessage(mine);

        SetDlgItemText(IDC_EDIT_CHAT_INPUT, _T(""));
    }
}

// ================================================================
//  OnBnClickedBtnChatBack  — 뒤로가기 버튼
// ================================================================
void ChatDlg::OnBnClickedBtnChatBack()
{
    ChatManager::GetInstance().UnregisterReceiveCallback();
    EndDialog(IDCANCEL);
}
