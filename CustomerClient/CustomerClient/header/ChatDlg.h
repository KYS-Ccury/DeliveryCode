#pragma once
#include "afxdialogex.h"
#include "ChatManager.h"

#define WM_CHAT_RECEIVED (WM_USER + 101)

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
    CString m_strTargetType;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void    OnBnClickedBtnChatBack();
    afx_msg void    OnBnClickedBtnChatSend();
    afx_msg LRESULT OnChatReceived(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
private:
    void AppendMessage(const ChatMessage& msg);
};
