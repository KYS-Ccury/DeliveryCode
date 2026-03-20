#include "pch.h"
#include "ChatDlg.h"
#include "Protocol.h"
#include "AppContext.h"

IMPLEMENT_DYNAMIC(ChatDlg, CDialogEx)

BEGIN_MESSAGE_MAP(ChatDlg, CDialogEx)
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

ChatDlg::~ChatDlg()
{
}

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

    // 상대방 표시
    CString title;
    if (m_partnerType == _T("ADMIN"))
        title = _T("도움요청 채팅 (관리자)");
    else
        title = _T("채팅 (고객)");
    SetWindowText(title);

    // 리스트박스 아이템 높이 (OwnerDraw)
    m_listChat.SetItemHeight(0, 48);

    // 채팅방 생성 또는 이력 요청
    CString payload;
    payload.Format(_T("%d|%s"), m_orderId, static_cast<LPCTSTR>(m_partnerType));
    bool bSent = AppContext::Get().socket.SendPacket(CMD_CHAT_CREATE, payload);

    if (bSent) {
        LoadChatHistory();
    } else {
        // 서버 미연결 시 더미 환영 메시지
        ChatMessage welcome;
        welcome.senderType = _T("ADMIN");
        welcome.message    = _T("안녕하세요! 무엇을 도와드릴까요?");
        welcome.sentAt     = _T("지금");
        welcome.isMine     = false;
        AppendMessage(welcome);
    }

    m_editInput.SetFocus();
    return FALSE;
}

// Enter 키 → 전송
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

// ─────────────────────────────────────────────
// 전송 버튼
// ─────────────────────────────────────────────
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

// ─────────────────────────────────────────────
// 메시지 전송
// ─────────────────────────────────────────────
void ChatDlg::SendMessage(const CString& text)
{
    // 내 메시지 즉시 화면에 추가
    ChatMessage msg;
    msg.senderType = _T("RIDER");
    msg.message    = text;
    msg.sentAt     = _T("방금");
    msg.isMine     = true;
    AppendMessage(msg);

    // 서버 전송
    CString payload;
    payload.Format(_T("%d|%s|%s"), m_orderId,
                   static_cast<LPCTSTR>(m_partnerType),
                   static_cast<LPCTSTR>(text));
    AppContext::Get().socket.SendPacket(CMD_CHAT_SEND, payload);
}

// ─────────────────────────────────────────────
// 메시지 리스트박스에 추가
// ─────────────────────────────────════════════
void ChatDlg::AppendMessage(const ChatMessage& msg)
{
    m_messages.Add(const_cast<ChatMessage&>(msg));

    // 리스트박스에는 인덱스 문자열 저장 (실제 렌더링은 OnDrawItem)
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

// ─────────────────────────────────────────────
// 채팅 이력 요청
// ─────────────────────────────────────────────
void ChatDlg::LoadChatHistory()
{
    CString payload;
    payload.Format(_T("%d|%s"), m_orderId, static_cast<LPCTSTR>(m_partnerType));
    AppContext::Get().socket.SendPacket(CMD_CHAT_HISTORY, payload);
}

// ─────────────────────────────────────────────
// 실시간 채팅 수신 (WM_CHAT_RECV)
// ─────────────────────────────────────────────
LRESULT ChatDlg::OnChatRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString payload = *pMsg;
    delete pMsg;

    ParseChatRecv(payload);
    return 0;
}

// ─────────────────────────────────────────────
// 소켓 수신 (WM_SOCKET_RECV)
// ─────────────────────────────────────────────
LRESULT ChatDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    int cmd = _ttoi(msg.Left(p));

    if (cmd == CMD_CHAT_HISTORY) {
        // "602|OK|payload"
        int p2 = msg.Find(_T('|'), p + 1);
        if (p2 >= 0 && msg.Mid(p + 1, p2 - p - 1) == _T("OK"))
            ParseHistoryResponse(msg.Mid(p2 + 1));
    }
    return 0;
}

// ─────────────────────────────────────────────
// 실시간 메시지 파싱
// payload: "sender|message|sentAt"
// ─────────────────────────────────────────────
void ChatDlg::ParseChatRecv(const CString& payload)
{
    CString data = payload;

    auto nextField = [&](CString& out) {
        int pipe = data.Find(_T('|'));
        if (pipe >= 0) {
            out  = data.Left(pipe);
            data = data.Mid(pipe + 1);
        } else {
            out  = data;
            data = _T("");
        }
    };

    ChatMessage msg;
    CString sender;
    nextField(sender);
    nextField(msg.message);
    nextField(msg.sentAt);

    msg.senderType = sender;
    msg.isMine     = (sender == _T("RIDER"));

    AppendMessage(msg);
}

// ─────────────────────────────────────────────
// 이력 파싱 (';' 구분)
// ─────────────────────────────────────────────
void ChatDlg::ParseHistoryResponse(const CString& payload)
{
    CString data = payload;
    while (!data.IsEmpty()) {
        int semi = data.Find(_T(';'));
        CString item = (semi >= 0) ? data.Left(semi) : data;
        data = (semi >= 0) ? data.Mid(semi + 1) : _T("");
        if (!item.IsEmpty())
            ParseChatRecv(item);
    }
}

// ─────────────────────────────────────────────
// Owner-Draw: 말풍선 스타일 렌더링
// ─────────────────────────────────────────────
void ChatDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    if (nIDCtl != IDC_LIST_CHAT || lpDIS->itemID == (UINT)-1) {
        CDialogEx::OnDrawItem(nIDCtl, lpDIS);
        return;
    }

    CDC* pDC = CDC::FromHandle(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);

    // 인덱스 파싱
    TCHAR buf[16] = {};
    m_listChat.GetText(lpDIS->itemID, buf);
    int idx = _ttoi(buf);
    if (idx < 0 || idx >= (int)m_messages.GetSize()) return;

    const ChatMessage& msg = m_messages[idx];

    // 배경
    pDC->FillSolidRect(&rc, GetSysColor(COLOR_WINDOW));

    // 폰트
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    pDC->SelectObject(hFont);

    const int BUBBLE_MARGIN = 8;
    const int BUBBLE_PAD    = 6;
    const int MAX_W         = rc.Width() * 6 / 10;

    // 텍스트 크기 계산
    CRect textRect(0, 0, MAX_W, 0);
    pDC->DrawText(msg.message, &textRect, DT_CALCRECT | DT_WORDBREAK);

    int bubbleW = textRect.Width()  + BUBBLE_PAD * 2;
    int bubbleH = textRect.Height() + BUBBLE_PAD * 2;
    int bubbleY = rc.top + (rc.Height() - bubbleH) / 2;

    CRect bubbleRect;
    if (msg.isMine) {
        // 오른쪽 정렬 (민트색)
        bubbleRect.SetRect(
            rc.right - BUBBLE_MARGIN - bubbleW,
            bubbleY,
            rc.right - BUBBLE_MARGIN,
            bubbleY + bubbleH);

        CBrush brush(RGB(29, 158, 117));
        CBrush* pOld = pDC->SelectObject(&brush);
        pDC->RoundRect(bubbleRect, CPoint(10, 10));
        pDC->SelectObject(pOld);

        pDC->SetTextColor(RGB(255, 255, 255));
    } else {
        // 왼쪽 정렬 (회색)
        bubbleRect.SetRect(
            rc.left + BUBBLE_MARGIN,
            bubbleY,
            rc.left + BUBBLE_MARGIN + bubbleW,
            bubbleY + bubbleH);

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

    // 시간 표시 (작게)
    if (!msg.sentAt.IsEmpty()) {
        pDC->SetTextColor(RGB(150, 150, 150));
        LOGFONT lf = {};
        GetObject(hFont, sizeof(lf), &lf);
        lf.lfHeight = -10;
        CFont smallFont;
        smallFont.CreateFontIndirect(&lf);
        pDC->SelectObject(&smallFont);

        CRect timeRect;
        if (msg.isMine) {
            timeRect.SetRect(bubbleRect.left - 40, bubbleRect.bottom - 14,
                             bubbleRect.left - 2,  bubbleRect.bottom);
        } else {
            timeRect.SetRect(bubbleRect.right + 2,  bubbleRect.bottom - 14,
                             bubbleRect.right + 42, bubbleRect.bottom);
        }
        pDC->DrawText(msg.sentAt, &timeRect, DT_SINGLELINE | DT_LEFT);
    }
}
