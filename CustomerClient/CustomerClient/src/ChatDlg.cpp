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
    ON_MESSAGE(WM_CHAT_ROOM_READY,   &ChatDlg::OnChatRoomReady)
END_MESSAGE_MAP()

// ================================================================
//  OnInitDialog
// ================================================================
BOOL ChatDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetDlgItemText(IDC_STATIC_CHAT_TITLE,
        m_strTargetName.IsEmpty() ? _T("채팅") : m_strTargetName);

    // SysListView32 초기화
    CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (pList) {
        CRect rc;
        pList->GetClientRect(&rc);
        pList->InsertColumn(0, _T(""), LVCFMT_LEFT, rc.Width());
        pList->SetExtendedStyle(pList->GetExtendedStyle() | LVS_EX_FULLROWSELECT);
    }

    auto& cm = ChatManager::GetInstance();
    cm.RegisterReceiveCallback(GetSafeHwnd());

    std::string targetType = "admin";
    if (!m_strTargetType.IsEmpty()) {
        std::string t = std::string(CT2A(m_strTargetType, CP_UTF8));
        if (t == "owner" || t == "store") targetType = "owner";
    }
    cm.CreateOrGetRoom(targetType, m_nOrderId);

    // 연결 중 안내 (WM_CHAT_ROOM_READY 수신 시 제거됨)
    AppendSystemMsg(_T("채팅방에 연결 중입니다..."));

    return TRUE;
}

// ================================================================
//  WM_CHAT_ROOM_READY  — 채팅방 연결 완료 (600 응답 수신 후)
//  "연결 중..." 텍스트를 지우고 과거메시지를 기다림
// ================================================================
LRESULT ChatDlg::OnChatRoomReady(WPARAM, LPARAM)
{
    ClearMessages();
    // 과거메시지는 WM_CHAT_HISTORY 로 곧 도착함
    // (602 응답이 빈 경우 대비 안내문은 OnChatHistory 에서 표시)
    return 0;
}

// ================================================================
//  helpers
// ================================================================
void ChatDlg::ClearMessages()
{
    CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (pList) pList->DeleteAllItems();
}

void ChatDlg::AppendSystemMsg(const CString& text)
{
    CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (!pList) return;
    int idx = pList->InsertItem(pList->GetItemCount(), text);
    pList->EnsureVisible(idx, FALSE);
}

void ChatDlg::AppendMessage(const ChatMessage& msg)
{
    CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (!pList) return;

    CString strMsg  = CA2T(msg.message.c_str(),   CP_UTF8);
    CString strTime = CA2T(msg.timestamp.c_str(), CP_UTF8);
    CString line;

    if (msg.isMine) {
        if (strTime.IsEmpty())
            line.Format(_T("  [나] %s"),       (LPCTSTR)strMsg);
        else
            line.Format(_T("  [나] %s  (%s)"), (LPCTSTR)strMsg, (LPCTSTR)strTime);
    } else {
        CString strRole = CA2T(msg.senderRole.c_str(), CP_UTF8);
        if (strRole.IsEmpty()) strRole = _T("상대방");
        if (strTime.IsEmpty())
            line.Format(_T("[%s] %s"),           (LPCTSTR)strRole, (LPCTSTR)strMsg);
        else
            line.Format(_T("[%s] %s  (%s)"),     (LPCTSTR)strRole, (LPCTSTR)strMsg,
                                                 (LPCTSTR)strTime);
    }

    int idx = pList->InsertItem(pList->GetItemCount(), line);
    pList->EnsureVisible(idx, FALSE);
}

// ================================================================
//  WM_CHAT_HISTORY  — 과거 메시지 일괄 수신
// ================================================================
LRESULT ChatDlg::OnChatHistory(WPARAM, LPARAM lParam)
{
    std::vector<ChatMessage>* pList =
        reinterpret_cast<std::vector<ChatMessage>*>(lParam);
    if (!pList) return 0;

    ClearMessages();
    if (pList->empty()) {
        AppendSystemMsg(_T("대화 내역이 없습니다."));
    } else {
        for (const auto& msg : *pList)
            AppendMessage(msg);
    }
    delete pList;
    return 0;
}

// ================================================================
//  WM_CHAT_RECEIVED  — 실시간 메시지 (상대방만 표시)
// ================================================================
LRESULT ChatDlg::OnChatReceived(WPARAM, LPARAM lParam)
{
    ChatMessage* pMsg = reinterpret_cast<ChatMessage*>(lParam);
    if (pMsg) {
        if (!pMsg->isMine)
            AppendMessage(*pMsg);
        delete pMsg;
    }
    return 0;
}

// ================================================================
//  전송 버튼 / 엔터키
// ================================================================
void ChatDlg::OnBnClickedBtnChatSend()
{
    CString strInput;
    GetDlgItemText(IDC_EDIT_CHAT_INPUT, strInput);
    strInput.Trim();
    if (strInput.IsEmpty()) return;

    std::string msg = std::string(CT2A(strInput, CP_UTF8));

    SetDlgItemText(IDC_EDIT_CHAT_INPUT, _T(""));
    GetDlgItem(IDC_EDIT_CHAT_INPUT)->SetFocus();

    // 내 메시지 즉시 로컬 표시
    ChatMessage mine;
    mine.senderRole = "CUSTOMER";
    mine.senderID   = AuthManager::GetInstance().GetCurrentUserID();
    mine.message    = msg;
    mine.isMine     = true;
    AppendMessage(mine);

    ChatManager::GetInstance().SendMessage(msg);
}

// ================================================================
//  뒤로가기 / ESC
// ================================================================
void ChatDlg::OnBnClickedBtnChatBack()
{
    ChatManager::GetInstance().UnregisterReceiveCallback();
    EndDialog(IDCANCEL);
}
