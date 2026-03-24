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
    ON_MESSAGE(WM_CHAT_RECEIVED,      &ChatDlg::OnChatReceived)
END_MESSAGE_MAP()

BOOL ChatDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetDlgItemText(IDC_STATIC_CHAT_TITLE,
        m_strTargetName.IsEmpty() ? _T("") : m_strTargetName);

    auto& cm = ChatManager::GetInstance();
    if (!cm.IsConnected()) cm.ConnectToServer("10.10.10.122", 8080);

    // CT2A 결과를 std::string 변수에 먼저 담기 (직접 전달 금지)
    std::string targetID   = m_strTargetID.IsEmpty()
                             ? "admin"
                             : std::string(CT2A(m_strTargetID, CP_UTF8));
    std::string targetType = m_strTargetType.IsEmpty()
                             ? "store"
                             : std::string(CT2A(m_strTargetType, CP_UTF8));

    cm.CreateOrGetRoom(targetID, targetType);
    cm.RegisterReceiveCallback(GetSafeHwnd());

    auto history = cm.GetChatHistory(targetID);
    for (const auto& msg : history) AppendMessage(msg);
    return TRUE;
}

void ChatDlg::AppendMessage(const ChatMessage& msg)
{
    CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_MSGS);
    if (!pList) return;
    // CA2T 결과를 CString 변수에 먼저 담기
    CString strSender  = CA2T(msg.senderID.c_str(), CP_UTF8);
    CString strMessage = CA2T(msg.message.c_str(),  CP_UTF8);
    CString line;
    if (msg.isMine)
        line.Format(_T("[Me] %s"), (LPCTSTR)strMessage);
    else
        line.Format(_T("[%s] %s"), (LPCTSTR)strSender, (LPCTSTR)strMessage);
    int idx = pList->AddString(line);
    pList->SetCurSel(idx);
}

void ChatDlg::OnBnClickedBtnChatSend()
{
    CString strInput;
    GetDlgItemText(IDC_EDIT_CHAT_INPUT, strInput);
    if (strInput.IsEmpty()) return;

    // CT2A -> std::string 변환 (직접 함수 인자로 전달 금지)
    std::string msg      = std::string(CT2A(strInput,       CP_UTF8));
    std::string targetID = std::string(CT2A(m_strTargetID,  CP_UTF8));

    bool sent = ChatManager::GetInstance().SendMessageTo(targetID, msg);
    if (sent) {
        ChatMessage cm;
        cm.senderID = AuthManager::GetInstance().GetCurrentUserID();
        cm.message  = msg;
        cm.isMine   = true;
        AppendMessage(cm);
        SetDlgItemText(IDC_EDIT_CHAT_INPUT, _T(""));
    }
}

LRESULT ChatDlg::OnChatReceived(WPARAM wParam, LPARAM lParam)
{
    ChatMessage* pMsg = reinterpret_cast<ChatMessage*>(lParam);
    if (pMsg) { AppendMessage(*pMsg); delete pMsg; }
    return 0;
}

void ChatDlg::OnBnClickedBtnChatBack()
{
    ChatManager::GetInstance().UnregisterReceiveCallback();
    EndDialog(IDCANCEL);
}
