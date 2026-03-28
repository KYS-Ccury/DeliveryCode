/**
 * PageInquiry.cpp
 * ============================================================
 * ★ 수정사항:
 *   1) 소켓 Lock/Unlock 추가 (폴링 스레드와 경합 방지)
 *   2) 방 선택 시 SetPollingRoomId() 호출 (폴링 대상 방 설정)
 *   3) OnPollRefreshChat() 추가 (폴링 새 메시지 수신 시 호출)
 *   4) 기존 채팅 UI / 서버 연동 로직 100% 유지
 * ============================================================
 */

#include "pch.h"
#include "PageInquiry.h"
#include "admintool.h"
#include "PacketDef.h"

 // ============================================================================
 // CChatPanel (기존 코드 그대로)
 // ============================================================================

BEGIN_MESSAGE_MAP(CChatPanel, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_SIZE()
END_MESSAGE_MAP()

CChatPanel::CChatPanel()
    : m_nScrollPos(0)
    , m_nTotalHeight(0)
{
}

CChatPanel::~CChatPanel()
{
}

void CChatPanel::AddMessage(const ChatMessage& msg)
{
    m_messages.push_back(msg);
    RecalcTotalHeight();
    ScrollToBottom();
    Invalidate(FALSE);
}

void CChatPanel::ClearMessages()
{
    m_messages.clear();
    m_nScrollPos = 0;
    m_nTotalHeight = 0;
    if (GetSafeHwnd())
    {
        SetScrollPos(SB_VERT, 0);
        Invalidate(FALSE);
    }
}

void CChatPanel::ScrollToBottom()
{
    if (!GetSafeHwnd()) return;
    CRect rc;
    GetClientRect(&rc);
    int nMax = m_nTotalHeight - rc.Height();
    if (nMax < 0) nMax = 0;
    m_nScrollPos = nMax;
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = m_nTotalHeight;
    si.nPage = (UINT)rc.Height();
    si.nPos = m_nScrollPos;
    SetScrollInfo(SB_VERT, &si, TRUE);
}

int CChatPanel::CalcMessageHeight(CDC* pDC, const CString& text, int nMaxWidth)
{
    CRect rcCalc(0, 0, nMaxWidth - BUBBLE_PADDING * 2, 0);
    pDC->DrawText(text, &rcCalc, DT_CALCRECT | DT_WORDBREAK | DT_LEFT);
    return rcCalc.Height() + BUBBLE_PADDING * 2;
}

void CChatPanel::RecalcTotalHeight()
{
    if (!GetSafeHwnd()) return;
    CClientDC dc(this);
    if (!m_font.GetSafeHandle())
        m_font.CreatePointFont(100, _T("맑은 고딕"));
    CFont* pOldFont = dc.SelectObject(&m_font);
    int nY = BUBBLE_MARGIN;
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        int nH = CalcMessageHeight(&dc, m_messages[i].text, MSG_MAX_WIDTH);
        nY += nH + BUBBLE_MARGIN;
    }
    m_nTotalHeight = nY;
    dc.SelectObject(pOldFont);
    CRect rc;
    GetClientRect(&rc);
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = m_nTotalHeight;
    si.nPage = (UINT)rc.Height();
    si.nPos = m_nScrollPos;
    SetScrollInfo(SB_VERT, &si, TRUE);
}

BOOL CChatPanel::OnEraseBkgnd(CDC* pDC) { return TRUE; }

void CChatPanel::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);
    memDC.FillSolidRect(rcClient, RGB(233, 233, 233));
    if (!m_font.GetSafeHandle())
        m_font.CreatePointFont(100, _T("맑은 고딕"));
    CFont* pOldFont = memDC.SelectObject(&m_font);
    memDC.SetBkMode(TRANSPARENT);
    int nY = BUBBLE_MARGIN - m_nScrollPos;
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        const ChatMessage& msg = m_messages[i];
        CRect rcCalc(0, 0, MSG_MAX_WIDTH - BUBBLE_PADDING * 2, 0);
        memDC.DrawText(msg.text, &rcCalc, DT_CALCRECT | DT_WORDBREAK | DT_LEFT);
        int nTextW = rcCalc.Width();
        int nTextH = rcCalc.Height();
        int nBubbleW = nTextW + BUBBLE_PADDING * 2;
        int nBubbleH = nTextH + BUBBLE_PADDING * 2;
        CRect rcBubble;
        if (msg.isAdmin)
        {
            int nRight = rcClient.right - 12;
            rcBubble.SetRect(nRight - nBubbleW, nY, nRight, nY + nBubbleH);
        }
        else
        {
            int nLeft = 12;
            rcBubble.SetRect(nLeft, nY, nLeft + nBubbleW, nY + nBubbleH);
        }
        if (rcBubble.bottom >= 0 && rcBubble.top <= rcClient.bottom)
        {
            COLORREF clrBubble = msg.isAdmin ? RGB(254, 235, 52) : RGB(255, 255, 255);
            CBrush brush(clrBubble);
            CPen pen(PS_SOLID, 1, clrBubble);
            CBrush* pOldBr = memDC.SelectObject(&brush);
            CPen* pOldPn = memDC.SelectObject(&pen);
            memDC.RoundRect(rcBubble, CPoint(BUBBLE_RADIUS, BUBBLE_RADIUS));
            memDC.SelectObject(pOldBr);
            memDC.SelectObject(pOldPn);
            memDC.SetTextColor(RGB(0, 0, 0));
            CRect rcText(
                rcBubble.left + BUBBLE_PADDING,
                rcBubble.top + BUBBLE_PADDING,
                rcBubble.right - BUBBLE_PADDING,
                rcBubble.bottom - BUBBLE_PADDING
            );
            memDC.DrawText(msg.text, &rcText, DT_WORDBREAK | DT_LEFT);
        }
        nY += nBubbleH + BUBBLE_MARGIN;
    }
    memDC.SelectObject(pOldFont);
    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void CChatPanel::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CRect rc;
    GetClientRect(&rc);
    int nMaxPos = m_nTotalHeight - rc.Height();
    if (nMaxPos < 0) nMaxPos = 0;
    switch (nSBCode)
    {
    case SB_LINEUP:        m_nScrollPos -= 20; break;
    case SB_LINEDOWN:      m_nScrollPos += 20; break;
    case SB_PAGEUP:        m_nScrollPos -= rc.Height(); break;
    case SB_PAGEDOWN:      m_nScrollPos += rc.Height(); break;
    case SB_THUMBTRACK:    m_nScrollPos = (int)nPos; break;
    case SB_THUMBPOSITION: m_nScrollPos = (int)nPos; break;
    }
    if (m_nScrollPos < 0) m_nScrollPos = 0;
    if (m_nScrollPos > nMaxPos) m_nScrollPos = nMaxPos;
    SetScrollPos(SB_VERT, m_nScrollPos);
    Invalidate(FALSE);
}

BOOL CChatPanel::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    CRect rc;
    GetClientRect(&rc);
    int nMaxPos = m_nTotalHeight - rc.Height();
    if (nMaxPos < 0) nMaxPos = 0;
    m_nScrollPos -= zDelta / 3;
    if (m_nScrollPos < 0) m_nScrollPos = 0;
    if (m_nScrollPos > nMaxPos) m_nScrollPos = nMaxPos;
    SetScrollPos(SB_VERT, m_nScrollPos);
    Invalidate(FALSE);
    return TRUE;
}

void CChatPanel::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    RecalcTotalHeight();
    Invalidate(FALSE);
}

// ============================================================================
// PageInquiry
// ============================================================================

IMPLEMENT_DYNAMIC(PageInquiry, PageBase)

static CClientSocket& GetSocket()
{
    return ((CAdminToolApp*)AfxGetApp())->GetSocket();
}

PageInquiry::PageInquiry(CWnd* pParent)
    : PageBase(IDD_PAGE_INQUIRY, pParent)
{
}

PageInquiry::~PageInquiry()
{
}

void PageInquiry::DoDataExchange(CDataExchange* pDX)
{
    PageBase::DoDataExchange(pDX);
}

BOOL PageInquiry::OnInitDialog()
{
    PageBase::OnInitDialog();
    m_font.CreatePointFont(120, _T("맑은 고딕"));
    InitRoomList();
    InitChatArea();
    return TRUE;
}

void PageInquiry::InitRoomList()
{
    CRect rcClient;
    GetClientRect(&rcClient);
    m_roomList.Create(
        NULL, _T("ChatRoomList"),
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
        CRect(0, 0, ROOM_LIST_W, rcClient.bottom),
        this, 5002
    );
    m_roomList.m_fnOnSelectChanged = [this](int nIndex)
        {
            const ChatRoomItem* pRoom = m_roomList.GetSelectedRoom();
            if (pRoom)
            {
                m_strCurrentRoomId = pRoom->orderId;
                RefreshChatFromServer(m_strCurrentRoomId);

                // ★ 폴링 대상 방 ID 설정
                CClientSocket& sock = GetSocket();
                std::string roomIdUtf8 = CStringToUtf8(m_strCurrentRoomId);
                sock.SetPollingRoomId(roomIdUtf8);
            }
        };
}

void PageInquiry::InitChatArea()
{
    CRect rcClient;
    GetClientRect(&rcClient);
    int nLeft = ROOM_LIST_W + 5;
    int nTop = 5;
    int nRight = rcClient.right - 5;
    int nBottom = rcClient.bottom - INPUT_H - 10;
    m_chatPanel.Create(
        NULL, _T("ChatPanel"),
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
        CRect(nLeft, nTop, nRight, nBottom),
        this, 5001
    );
    m_editChatInput.Create(
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        CRect(nLeft, nBottom + 5, nRight - SEND_BTN_W - 5, nBottom + 5 + INPUT_H),
        this, EDIT_ID
    );
    m_editChatInput.SetFont(&m_font);
    m_btnSend.Create(
        _T("Send"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(nRight - SEND_BTN_W, nBottom + 5, nRight, nBottom + 5 + INPUT_H),
        this, SEND_ID
    );
    m_btnSend.SetFont(&m_font);
}

void PageInquiry::UpdateLayout()
{
    if (!IsWindow(m_chatPanel.GetSafeHwnd())) return;
    CRect rcClient;
    GetClientRect(&rcClient);
    int nLeft = ROOM_LIST_W + 5;
    int nTop = 5;
    int nRight = rcClient.right - 5;
    int nBottom = rcClient.bottom - INPUT_H - 10;
    if (IsWindow(m_roomList.GetSafeHwnd()))
        m_roomList.MoveWindow(0, 0, ROOM_LIST_W, rcClient.bottom);
    m_chatPanel.MoveWindow(nLeft, nTop, nRight - nLeft, nBottom - nTop);
    if (IsWindow(m_editChatInput.GetSafeHwnd()))
        m_editChatInput.MoveWindow(nLeft, nBottom + 5, nRight - nLeft - SEND_BTN_W - 5, INPUT_H);
    if (IsWindow(m_btnSend.GetSafeHwnd()))
        m_btnSend.MoveWindow(nRight - SEND_BTN_W, nBottom + 5, SEND_BTN_W, INPUT_H);
}

// ============================================================
// ★ 서버에서 채팅방 목록 로드 (CMD_ROOM_LIST = 603)
//    Lock/Unlock 추가
// ============================================================
void PageInquiry::LoadChatRoomsFromServer()
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected())
    {
        m_roomList.ClearRooms();
        return;
    }

    sock.Lock();  // ★

    json reqBody;
    if (!sock.SendAdminPacket(CMD_ROOM_LIST, reqBody))
    {
        sock.Unlock();
        m_roomList.ClearRooms();
        return;
    }

    RecvResult res = sock.RecvPacket();

    sock.Unlock();  // ★

    if (!res.success)
    {
        m_roomList.ClearRooms();
        return;
    }

    std::vector<ChatRoomItem> rooms;

    if (res.body.contains("rooms") && res.body["rooms"].is_array())
    {
        for (auto& room : res.body["rooms"])
        {
            ChatRoomItem item;
            item.orderId = Utf8ToCString(room.value("room_id", ""));
            item.lastMessage = Utf8ToCString(room.value("last_message", ""));
            item.isSelected = false;

            std::string roomType = room.value("room_type", "");

            // ★ 사장↔고객 1:1 채팅방은 관리자에게 표시하지 않음
            if (roomType == "CUSTOMER_OWNER")
                continue;

            if (roomType == "CUSTOMER_ADMIN")
                item.role = _T("고객문의");
            else if (roomType == "RIDER_ADMIN")
                item.role = _T("라이더문의");
            else
                item.role = Utf8ToCString(roomType);

            std::string custName = room.value("customer_name", "");
            if (!custName.empty())
            {
                CString strName = Utf8ToCString(custName);
                item.role.Format(_T("%s (%s)"), (LPCTSTR)item.role, (LPCTSTR)strName);
            }

            rooms.push_back(item);
        }
    }

    m_roomList.SetRooms(rooms);
}

// ============================================================
// ★ 서버에서 특정 채팅방 메시지 조회 (CMD_GET_MSGS = 602)
//    Lock/Unlock 추가
// ============================================================
void PageInquiry::RefreshChatFromServer(const CString& strRoomId)
{
    m_chatPanel.ClearMessages();

    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    std::string roomIdUtf8 = CStringToUtf8(strRoomId);

    sock.Lock();  // ★

    json reqBody;
    reqBody["room_id"] = roomIdUtf8;

    if (!sock.SendAdminPacket(CMD_GET_MSGS, reqBody))
    {
        sock.Unlock();
        return;
    }

    RecvResult res = sock.RecvPacket();

    sock.Unlock();  // ★

    if (!res.success) return;

    if (res.body.contains("messages") && res.body["messages"].is_array())
    {
        for (auto& msg : res.body["messages"])
        {
            ChatMessage chatMsg;
            chatMsg.text = Utf8ToCString(msg.value("text", ""));
            chatMsg.isAdmin = msg.value("is_admin", false);
            m_chatPanel.AddMessage(chatMsg);
        }
    }
}

// ============================================================
// ★ 서버로 메시지 전송 (CMD_SEND_MSG = 601)
//    Lock/Unlock 추가
// ============================================================
void PageInquiry::SendMessageToServer(const CString& strRoomId, const CString& strMsg)
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    std::string roomIdUtf8 = CStringToUtf8(strRoomId);
    std::string msgUtf8 = CStringToUtf8(strMsg);

    sock.Lock();  // ★

    json reqBody;
    reqBody["room_id"] = roomIdUtf8;
    reqBody["message"] = msgUtf8;

    if (!sock.SendAdminPacket(CMD_SEND_MSG, reqBody))
    {
        sock.Unlock();
        AfxMessageBox(_T("메시지 전송 실패"));
        return;
    }

    RecvResult res = sock.RecvPacket();

    sock.Unlock();  // ★

    if (res.success)
    {
        ChatMessage chatMsg;
        chatMsg.text = strMsg;
        chatMsg.isAdmin = true;
        m_chatPanel.AddMessage(chatMsg);

        int nSel = m_roomList.GetSelectedIndex();
        if (nSel >= 0)
            m_roomList.UpdateLastMessage(nSel, strMsg);
    }
    else
    {
        AfxMessageBox(_T("메시지 전송 응답 실패"));
    }
}

// ============================================================
// ★ 폴링에서 새 메시지 도착 시 호출 (MainDialog → 여기)
// ============================================================
void PageInquiry::OnPollRefreshChat()
{
    if (!m_strCurrentRoomId.IsEmpty())
    {
        RefreshChatFromServer(m_strCurrentRoomId);
    }
}

// ============================================================
// LoadData: 페이지 진입 시
// ============================================================
void PageInquiry::LoadData()
{
    m_strCurrentRoomId.Empty();
    if (IsWindow(m_chatPanel.GetSafeHwnd()))
        m_chatPanel.ClearMessages();

    // ★ 폴링 방 ID 초기화
    CClientSocket& sock = GetSocket();
    sock.SetPollingRoomId("");

    LoadChatRoomsFromServer();
}

void PageInquiry::SaveData()
{
    AfxMessageBox(_T("Inquiry Saved"));
}

void PageInquiry::OnSize(UINT nType, int cx, int cy)
{
    PageBase::OnSize(nType, cx, cy);
    UpdateLayout();
}

void PageInquiry::OnBtnChatSend()
{
    if (m_strCurrentRoomId.IsEmpty())
    {
        AfxMessageBox(_T("채팅방을 먼저 선택하세요"));
        return;
    }

    CString strInput;
    m_editChatInput.GetWindowText(strInput);
    strInput.Trim();
    if (strInput.IsEmpty())
    {
        AfxMessageBox(_T("메시지를 입력하세요"));
        return;
    }

    SendMessageToServer(m_strCurrentRoomId, strInput);
    m_editChatInput.SetWindowText(_T(""));
    m_editChatInput.SetFocus();
}

BOOL PageInquiry::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        if (IsWindow(m_editChatInput.GetSafeHwnd()))
        {
            CWnd* pFocus = GetFocus();
            if (pFocus && pFocus->GetSafeHwnd() == m_editChatInput.GetSafeHwnd())
            {
                OnBtnChatSend();
                return TRUE;
            }
        }
    }
    return PageBase::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(PageInquiry, PageBase)
    ON_BN_CLICKED(SEND_ID, &PageInquiry::OnBtnChatSend)
    ON_WM_SIZE()
END_MESSAGE_MAP()