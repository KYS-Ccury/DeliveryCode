#pragma once

#include "PageBase.h"
#include <vector>
#include <string>
#include <functional>

/**
 * PageInquiry.h
 * ============================================================
 * 1:1 문의(채팅) 페이지이다.
 * 좌측에 채팅방 목록, 우측에 채팅 패널 + 입력창을 배치한다.
 *
 * ★ 서버 연동:
 *   CMD_SEND_MSG  (601) - 메시지 전송
 *   CMD_GET_MSGS  (602) - 메시지 조회
 *   CMD_ROOM_LIST (603) - 채팅방 목록 조회
 * ============================================================
 */

 // ============================================================
 // 채팅 메시지 구조체
 // ============================================================
struct ChatMessage
{
    CString text;       // 메시지 내용
    bool    isAdmin;    // true = 관리자(오른쪽), false = 고객(왼쪽)
};

// ============================================================
// 채팅방 아이템 구조체
// ============================================================
struct ChatRoomItem
{
    CString orderId;        // 채팅방 식별자 (room_id)
    CString role;           // 역할 (예: "고객")
    CString lastMessage;    // 마지막 메시지
    bool    isSelected;     // 선택 여부
};

// ============================================================
// CChatPanel - 채팅 말풍선을 자체 그리는 커스텀 윈도우
// ============================================================
class CChatPanel : public CWnd
{
public:
    CChatPanel();
    virtual ~CChatPanel();

    void AddMessage(const ChatMessage& msg);
    void ClearMessages();
    void ScrollToBottom();

protected:
    afx_msg void    OnPaint();
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg void    OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void    OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    void RecalcTotalHeight();
    int  CalcMessageHeight(CDC* pDC, const CString& text, int nMaxWidth);

    std::vector<ChatMessage> m_messages;
    CFont m_font;
    int   m_nScrollPos;
    int   m_nTotalHeight;

    // 상수
    static const int BUBBLE_PADDING = 10;
    static const int BUBBLE_MARGIN = 8;
    static const int BUBBLE_RADIUS = 10;
    static const int MSG_MAX_WIDTH = 320;
};

// ============================================================
// CChatRoomList - 좌측 채팅방 목록 커스텀 윈도우
// ============================================================
class CChatRoomList : public CWnd
{
public:
    CChatRoomList();
    virtual ~CChatRoomList();

    // 채팅방 목록 설정
    void SetRooms(const std::vector<ChatRoomItem>& rooms);

    // 선택된 채팅방 가져오기
    const ChatRoomItem* GetSelectedRoom() const;
    int GetSelectedIndex() const;

    // 마지막 메시지 갱신
    void UpdateLastMessage(int nIndex, const CString& strMsg);

    // 채팅방 선택 변경 콜백
    std::function<void(int)> m_fnOnSelectChanged;

private:
    std::vector<ChatRoomItem> m_rooms;
};

// ============================================================
// PageInquiry - 1:1 문의 페이지
// ============================================================
class PageInquiry : public PageBase
{
    DECLARE_DYNAMIC(PageInquiry)

public:
    PageInquiry(CWnd* pParent = nullptr);
    virtual ~PageInquiry();

    // PageBase 인터페이스
    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("1:1 문의"); }

    // 메시지 선처리 (Enter 키 → 전송)
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 메시지 핸들러
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnChatSend();

    DECLARE_MESSAGE_MAP()

private:
    // 초기화 / 레이아웃
    void InitRoomList();
    void InitChatArea();
    void UpdateLayout();

    // 서버 연동
    void LoadChatRoomsFromServer();                                     // CMD_ROOM_LIST (603)
    void RefreshChatFromServer(const std::wstring& strUser);            // CMD_GET_MSGS  (602)
    void SendMessageToServer(const std::wstring& strUser, const CString& strMsg);  // CMD_SEND_MSG  (601)

    // 현재 선택된 채팅방 사용자
    std::wstring m_strCurrentUser;

    // UI 컨트롤
    CChatRoomList m_roomList;
    CChatPanel    m_chatPanel;
    CEdit         m_editChatInput;
    CButton       m_btnSend;
    CFont         m_font;

    // 레이아웃 상수
    static const int ROOM_LIST_W = 200;
    static const int INPUT_H = 32;
    static const int SEND_BTN_W = 60;

    // 컨트롤 ID
    static const int EDIT_ID = 5003;
    static const int SEND_ID = 5004;
};