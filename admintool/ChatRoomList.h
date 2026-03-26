#pragma once

#include <afxwin.h>
#include <vector>
#include <functional>

// 채팅방 항목 구조체이다.
struct ChatRoomItem
{
    CString orderId;
    CString role;
    CString lastMessage;
    bool    isSelected;
};

// 카드형 채팅방 리스트 컨트롤이다.
class CChatRoomList : public CWnd
{
public:
    CChatRoomList();
    virtual ~CChatRoomList();

    // 채팅방 목록을 설정한다.
    void SetRooms(const std::vector<ChatRoomItem>& rooms);

    // 채팅방을 추가한다.
    void AddRoom(const ChatRoomItem& room);

    // 전체 목록을 지운다.
    void ClearRooms();

    // 선택된 인덱스를 반환한다.
    int GetSelectedIndex() const;

    // 선택된 항목을 반환한다.
    const ChatRoomItem* GetSelectedRoom() const;

    // 특정 항목의 마지막 메시지를 갱신한다.
    void UpdateLastMessage(int nIndex, const CString& strMsg);

    // 선택 변경 알림 콜백이다.
    std::function<void(int)> m_fnOnSelectChanged;

protected:
    std::vector<ChatRoomItem> m_rooms;

    int m_nSelectedIndex;
    int m_nHoverIndex;
    int m_nScrollPos;
    int m_nTotalHeight;

    CFont m_fontTitle;
    CFont m_fontMsg;

    // 카드 레이아웃 상수이다.
    static const int CARD_HEIGHT = 60;
    static const int CARD_MARGIN = 4;
    static const int CARD_PADDING = 10;
    static const int CARD_RADIUS = 8;
    static const int SIDE_MARGIN = 4;

    // 전체 높이를 재계산한다.
    void RecalcTotalHeight();

    // 스크롤 정보를 갱신한다.
    void UpdateScrollInfo();

    // 좌표로부터 카드 인덱스를 구한다.
    int HitTest(CPoint pt) const;

    // 스크롤을 맨 아래로 이동한다.
    void ScrollToBottom();

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    bool m_bTrackingMouse;
};