/**
 * PageInquiry.h
 * ============================================================
 * ★ 수정사항:
 *   1) CChatRoomList 중복 정의 제거 → ChatRoomList.h include (크래시 해결)
 *   2) 비동기 수신 스레드 제거 (소켓 경합 → 크래시 원인)
 *   3) UTF-8 → CString 변환 헬퍼 추가 (한글 깨짐 해결)
 *   4) ★ OnPollRefreshChat() 추가 (폴링으로 새 메시지 수신 시 호출)
 * ============================================================
 */

#pragma once

#include "PageBase.h"
#include "ChatRoomList.h"
#include <vector>
#include <string>
#include <functional>

 // ============================================================
 // ★ UTF-8 → CString 변환 헬퍼
 // ============================================================
inline CString Utf8ToCString(const std::string& utf8)
{
    if (utf8.empty()) return _T("");
    int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    if (nLen <= 0) return _T("");
    CString result;
    LPWSTR pBuf = result.GetBuffer(nLen);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), pBuf, nLen);
    result.ReleaseBuffer(nLen);
    return result;
}

// ============================================================
// ★ CString → UTF-8 std::string 변환 헬퍼
// ============================================================
inline std::string CStringToUtf8(const CString& str)
{
    if (str.IsEmpty()) return "";
    int nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)str, str.GetLength(), NULL, 0, NULL, NULL);
    if (nLen <= 0) return "";
    std::string result(nLen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)str, str.GetLength(), &result[0], nLen, NULL, NULL);
    return result;
}

// ============================================================
// 채팅 메시지 구조체
// ============================================================
struct ChatMessage
{
    CString text;
    bool    isAdmin;
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

    static const int BUBBLE_PADDING = 10;
    static const int BUBBLE_MARGIN = 8;
    static const int BUBBLE_RADIUS = 10;
    static const int MSG_MAX_WIDTH = 320;
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

    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("1:1 문의"); }

    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    // ★ 폴링에서 새 메시지 도착 시 MainDialog가 호출
    void OnPollRefreshChat();

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnChatSend();

    DECLARE_MESSAGE_MAP()

private:
    void InitRoomList();
    void InitChatArea();
    void UpdateLayout();

    void LoadChatRoomsFromServer();
    void RefreshChatFromServer(const CString& strRoomId);
    void SendMessageToServer(const CString& strRoomId, const CString& strMsg);

    CString m_strCurrentRoomId;

    CChatRoomList m_roomList;
    CChatPanel    m_chatPanel;
    CEdit         m_editChatInput;
    CButton       m_btnSend;
    CFont         m_font;

    static const int ROOM_LIST_W = 200;
    static const int INPUT_H = 32;
    static const int SEND_BTN_W = 60;

    static const int EDIT_ID = 5003;
    static const int SEND_ID = 5004;
};