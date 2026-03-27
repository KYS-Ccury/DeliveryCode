#pragma once
#include "afxdialogex.h"
#include "ChatManager.h"

class ChatDlg : public CDialogEx
{
    DECLARE_DYNAMIC(ChatDlg)
public:
    ChatDlg(CWnd* pParent = nullptr);
    virtual ~ChatDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_CHAT_DLG };
#endif

    CString m_strTargetName;
    CString m_strTargetID;
    CString m_strTargetType;   // "admin" | "owner"
    int     m_nOrderId = 0;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    virtual void OnOK()     override { OnBnClickedBtnChatSend(); }
    virtual void OnCancel() override { OnBnClickedBtnChatBack(); }

    afx_msg void    OnBnClickedBtnChatBack();
    afx_msg void    OnBnClickedBtnChatSend();
    afx_msg LRESULT OnChatReceived  (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnChatHistory   (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnChatRoomReady (WPARAM wParam, LPARAM lParam);  // 연결 완료

    DECLARE_MESSAGE_MAP()

private:
    void AppendMessage(const ChatMessage& msg);
    void AppendSystemMsg(const CString& text);
    void ClearMessages();

};
