#pragma once
#include <afxdialogex.h>
#include <afxcmn.h>

// ChatDlg.cpp 와 멤버명 일치
struct ChatMessage {
    bool    isMine     = false;
    CString senderType;   // "RIDER" | "ADMIN" | "CUSTOMER"
    CString message;
    CString sentAt;
};

class ChatDlg : public CDialogEx {
    DECLARE_DYNAMIC(ChatDlg)
public:
    // orderId  : 현재 주문 ID (0 이면 일반 관리자 채팅)
    // partnerType : "ADMIN" | "CUSTOMER"
    ChatDlg(int orderId, const CString& partnerType, CWnd* pParent = nullptr);
    virtual ~ChatDlg();
    enum { IDD = IDD_CHAT_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH m_hBrushBg = nullptr;
    CListBox m_listChat;
    CEdit    m_editInput;
    int      m_orderId     = 0;
    CString  m_partnerType;
    CArray<ChatMessage, const ChatMessage&> m_messages;

    afx_msg void    OnBtnSend();
    afx_msg void    OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
    afx_msg LRESULT OnChatRecv(WPARAM w, LPARAM l);

    void SendMessage(const CString& text);
    void AppendMessage(const ChatMessage& msg);
    void ScrollToBottom();
    void LoadChatHistory();
    void ParseChatRecv(const CString& payload);
    void ParseHistoryResponse(const CString& payload);
};
