#pragma once
#include <afxdialogex.h>
#include <afxcmn.h>

struct ChatMessage {
    bool    isMine     = false;
    CString senderType;   // "RIDER" | "ADMIN"
    CString message;
    CString sentAt;
};

class ChatDlg : public CDialogEx {
    DECLARE_DYNAMIC(ChatDlg)
public:
    // 관리자 채팅 전용 - 파라미터 없이 생성
    explicit ChatDlg(CWnd* pParent = nullptr);
    virtual ~ChatDlg();
    enum { IDD = IDD_CHAT_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
    HBRUSH   m_hBrushBg = nullptr;
    CListBox m_listChat;
    CEdit    m_editInput;
    int      m_roomId   = 0;   // 서버에서 받은 채팅방 ID
    CArray<ChatMessage, const ChatMessage&> m_messages;

    afx_msg void    OnBtnSend();
    afx_msg void    OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS);
    afx_msg void    OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
    afx_msg LRESULT OnChatRecv(WPARAM w, LPARAM l);

    void SendMessage(const CString& text);
    void AppendMessage(const ChatMessage& msg);
    void ScrollToBottom();
    void LoadChatHistory();
};
