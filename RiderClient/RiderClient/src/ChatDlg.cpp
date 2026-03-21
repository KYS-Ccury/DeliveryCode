// ChatDlg.cpp - Real-time chat with JSON binary protocol
#include "pch.h"
#include "ChatDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(ChatDlg, CDialogEx)

BEGIN_MESSAGE_MAP(ChatDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_SEND,   &ChatDlg::OnBtnSend)
    ON_MESSAGE(WM_CHAT_RECV,      &ChatDlg::OnChatRecv)
    ON_MESSAGE(WM_SOCKET_RECV,    &ChatDlg::OnSocketRecv)
    ON_WM_DRAWITEM()
END_MESSAGE_MAP()

ChatDlg::ChatDlg(int orderId, const CString& partnerType, CWnd* pParent)
    : CDialogEx(IDD_CHAT_DLG, pParent)
    , m_orderId(orderId)
    , m_partnerType(partnerType)
{
}

ChatDlg::~ChatDlg() {}

void ChatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_CHAT,  m_listChat);
    DDX_Control(pDX, IDC_EDIT_INPUT, m_editInput);
}

BOOL ChatDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    CString title = (m_partnerType == _T("ADMIN"))
                    ? _T("도움요청 채팅 (관리자)")
                    : _T("채팅 (고객)");
    SetWindowText(title);

    m_listChat.SetItemHeight(0, 48);

    // Create chat room: CMD_CHAT_CREATE(600), {"order_id":N,"partner":"ADMIN"}
    CT2A partUtf8(m_partnerType, CP_UTF8);
    json req;
    req["order_id"] = m_orderId;
    req["partner"]  = std::string(partUtf8);

    bool bSent = AppContext::Get().socket.SendPacket(CMD_CHAT_CREATE, req.dump());
    if (bSent) {
        LoadChatHistory();
    } else {
        // Offline dummy message
        ChatMessage welcome;
        welcome.senderType = _T("ADMIN");
        welcome.message    = _T("안녕하세요! 무엇을 도와드릴까요?");
        welcome.sentAt     = _T("방금");
        welcome.isMine     = false;
        AppendMessage(welcome);
    }

    m_editInput.SetFocus();
    return FALSE;
}

BOOL ChatDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
        if (GetFocus() == &m_editInput) {
            OnBtnSend();
            return TRUE;
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

void ChatDlg::OnBtnSend()
{
    CString text;
    m_editInput.GetWindowText(text);
    text.Trim();
    if (text.IsEmpty()) return;
    SendMessage(text);
    m_editInput.SetWindowText(_T(""));
    m_editInput.SetFocus();
}

// Send: CMD_CHAT_SEND(601), {"order_id":N,"partner":"...","message":"..."}
void ChatDlg::SendMessage(const CString& text)
{
    ChatMessage msg;
    msg.senderType = _T("RIDER");
    msg.message    = text;
    msg.sentAt     = _T("방금");
    msg.isMine     = true;
    AppendMessage(msg);

    CT2A partUtf8(m_partnerType, CP_UTF8);
    CT2A textUtf8(text, CP_UTF8);
    json req;
    req["order_id"] = m_orderId;
    req["partner"]  = std::string(partUtf8);
    req["message"]  = std::string(textUtf8);
    AppContext::Get().socket.SendPacket(CMD_CHAT_SEND, req.dump());
}

void ChatDlg::AppendMessage(const ChatMessage& msg)
{
    m_messages.Add(const_cast<ChatMessage&>(msg));
    CString idx;
    idx.Format(_T("%d"), (int)m_messages.GetSize() - 1);
    m_listChat.AddString(idx);
    ScrollToBottom();
}

void ChatDlg::ScrollToBottom()
{
    int cnt = m_listChat.GetCount();
    if (cnt > 0) m_listChat.SetTopIndex(cnt - 1);
}

// Request history: CMD_CHAT_HISTORY(602), {"order_id":N,"partner":"..."}
void ChatDlg::LoadChatHistory()
{
    CT2A partUtf8(m_partnerType, CP_UTF8);
    json req;
    req["order_id"] = m_orderId;
    req["partner"]  = std::string(partUtf8);
    AppContext::Get().socket.SendPacket(CMD_CHAT_HISTORY, req.dump());
}

// WM_CHAT_RECV (protocol 604): real-time message push
// JSON: {"sender":"ADMIN","message":"...","sent_at":"..."}
LRESULT ChatDlg::OnChatRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    std::string body = pPkt->body;
    delete pPkt;

    try {
        json res = json::parse(body);
        ChatMessage msg;

        std::string sender = res.value("sender", "");
        std::string text   = res.value("message", "");
        std::string sentAt = res.value("sent_at", "now");

        auto toCS = [](const std::string& s) -> CString {
            CA2T ws(s.c_str(), CP_UTF8); return CString(ws);
        };
        msg.senderType = toCS(sender);
        msg.message    = toCS(text);
        msg.sentAt     = toCS(sentAt);
        msg.isMine     = (sender == "RIDER");
        AppendMessage(msg);
    } catch (...) {}
    return 0;
}

// WM_SOCKET_RECV: chat history response
// JSON: {"status":2000,"messages":[{"sender":"...","message":"...","sent_at":"..."},...]}
LRESULT ChatDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    if (protocol != CMD_CHAT_HISTORY) return 0;

    try {
        json res = json::parse(body);
        if (res.value("status", 0) != STATUS_SUCCESS) return 0;

        auto toCS = [](const std::string& s) -> CString {
            CA2T ws(s.c_str(), CP_UTF8); return CString(ws);
        };

        for (const auto& m : res["messages"]) {
            ChatMessage msg;
            std::string sender = m.value("sender", "");
            msg.senderType = toCS(sender);
            msg.message    = toCS(m.value("message", ""));
            msg.sentAt     = toCS(m.value("sent_at",  ""));
            msg.isMine     = (sender == "RIDER");
            AppendMessage(msg);
        }
    } catch (...) {}
    return 0;
}

// Owner-Draw: chat bubble rendering
void ChatDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    if (nIDCtl != IDC_LIST_CHAT || lpDIS->itemID == (UINT)-1) {
        CDialogEx::OnDrawItem(nIDCtl, lpDIS);
        return;
    }

    CDC* pDC = CDC::FromHandle(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);

    TCHAR buf[16] = {};
    m_listChat.GetText(lpDIS->itemID, buf);
    int idx = _ttoi(buf);
    if (idx < 0 || idx >= (int)m_messages.GetSize()) return;

    const ChatMessage& msg = m_messages[idx];

    pDC->FillSolidRect(&rc, GetSysColor(COLOR_WINDOW));

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    pDC->SelectObject(hFont);

    const int BUBBLE_MARGIN = 8;
    const int BUBBLE_PAD    = 6;
    const int MAX_W         = rc.Width() * 6 / 10;

    CRect textRect(0, 0, MAX_W, 0);
    pDC->DrawText(msg.message, &textRect, DT_CALCRECT | DT_WORDBREAK);

    int bubbleW = textRect.Width()  + BUBBLE_PAD * 2;
    int bubbleH = textRect.Height() + BUBBLE_PAD * 2;
    int bubbleY = rc.top + (rc.Height() - bubbleH) / 2;

    CRect bubbleRect;
    if (msg.isMine) {
        bubbleRect.SetRect(rc.right - BUBBLE_MARGIN - bubbleW, bubbleY,
                           rc.right - BUBBLE_MARGIN, bubbleY + bubbleH);
        CBrush brush(RGB(29, 158, 117));
        CBrush* pOld = pDC->SelectObject(&brush);
        pDC->RoundRect(bubbleRect, CPoint(10, 10));
        pDC->SelectObject(pOld);
        pDC->SetTextColor(RGB(255, 255, 255));
    } else {
        bubbleRect.SetRect(rc.left + BUBBLE_MARGIN, bubbleY,
                           rc.left + BUBBLE_MARGIN + bubbleW, bubbleY + bubbleH);
        CBrush brush(RGB(235, 235, 235));
        CBrush* pOld = pDC->SelectObject(&brush);
        pDC->RoundRect(bubbleRect, CPoint(10, 10));
        pDC->SelectObject(pOld);
        pDC->SetTextColor(RGB(30, 30, 30));
    }

    pDC->SetBkMode(TRANSPARENT);
    CRect textDraw = bubbleRect;
    textDraw.DeflateRect(BUBBLE_PAD, BUBBLE_PAD);
    pDC->DrawText(msg.message, &textDraw, DT_WORDBREAK | DT_LEFT);

    if (!msg.sentAt.IsEmpty()) {
        pDC->SetTextColor(RGB(150, 150, 150));
        LOGFONT lf = {};
        GetObject(hFont, sizeof(lf), &lf);
        lf.lfHeight = -10;
        CFont smallFont;
        smallFont.CreateFontIndirect(&lf);
        pDC->SelectObject(&smallFont);

        CRect timeRect;
        if (msg.isMine)
            timeRect.SetRect(bubbleRect.left-40, bubbleRect.bottom-14, bubbleRect.left-2, bubbleRect.bottom);
        else
            timeRect.SetRect(bubbleRect.right+2, bubbleRect.bottom-14, bubbleRect.right+42, bubbleRect.bottom);
        pDC->DrawText(msg.sentAt, &timeRect, DT_SINGLELINE | DT_LEFT);
    }
}

HBRUSH ChatDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg) m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    return hbr;
}
