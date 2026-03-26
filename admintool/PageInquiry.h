#pragma once
#include "PageBase.h"
#include "ChatRoomList.h"
#include "resource.h"
#include <map>
#include <vector>
#include <string>
#include <functional>

struct ChatMessage
{
    bool    isAdmin;
    CString text;
};

class CChatPanel : public CWnd
{
public:
    CChatPanel();
    virtual ~CChatPanel();

    void AddMessage(const ChatMessage& msg);
    void ClearMessages();
    void ScrollToBottom();

protected:
    std::vector<ChatMessage> m_messages;
    CFont   m_font;
    CFont   m_fontBold;
    int     m_nScrollPos;
    int     m_nTotalHeight;

    static const int MSG_MAX_WIDTH = 250;
    static const int BUBBLE_PADDING = 8;
    static const int BUBBLE_MARGIN = 6;
    static const int BUBBLE_RADIUS = 10;

    int CalcMessageHeight(CDC* pDC, const CString& text, int nMaxWidth);
    void RecalcTotalHeight();

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()
};

class PageInquiry : public PageBase
{
    DECLARE_DYNAMIC(PageInquiry)

public:
    PageInquiry(CWnd* pParent = nullptr);
    virtual ~PageInquiry();

    enum { IDD = IDD_PAGE_INQUIRY };

    virtual CString GetPageName() override { return _T("Inquiry"); }
    virtual void LoadData() override;
    virtual void SaveData() override;

protected:
    CChatRoomList m_roomList;
    CChatPanel    m_chatPanel;
    CEdit         m_editChatInput;
    CButton       m_btnSend;
    CFont         m_font;

    std::map<std::wstring, std::vector<ChatMessage>> m_chatData;
    std::wstring m_strCurrentUser;

    static const int ROOM_LIST_W = 200;
    static const int INPUT_H = 30;
    static const int SEND_BTN_W = 70;
    static const int EDIT_ID = 5003;
    static const int SEND_ID = 5004;

    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;

    void InitRoomList();
    void InitChatArea();
    void LoadChatData();
    void RefreshChat(const std::wstring& strUser);
    void AddMessage(const std::wstring& strUser, const CString& strMsg, bool bAdmin);
    void UpdateLayout();

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnChatSend();

    DECLARE_MESSAGE_MAP()
};