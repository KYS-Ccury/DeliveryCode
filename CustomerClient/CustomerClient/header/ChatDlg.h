#pragma once
// ================================================================
//  ChatDlg.h  (과거 메시지 일괄 수신 지원 버전)
//
//  변경 사항:
//    - WM_CHAT_HISTORY (WM_USER+102) 핸들러 추가
//      → 서버에서 과거 메시지 목록을 받으면 리스트박스에 일괄 표시
//    - OnInitDialog 에서 ChatManager::RequestHistory() 직접 호출 제거
//      (ChatManager::CreateOrGetRoom 내부에서 자동 처리됨)
//    - m_strTargetType : "admin" | "owner" 로 통일
//    - m_nOrderId      : owner 채팅 시 order_id 전달용 (신규)
// ================================================================
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

    // ── 호출자가 설정하는 파라미터 ────────────────────────────
    CString m_strTargetName;   // 타이틀 표시용  ("관리자 문의" / 가게명)
    CString m_strTargetID;     // (호환용, 내부에서는 m_strTargetType 사용)
    CString m_strTargetType;   // "admin" | "owner"
    int     m_nOrderId = 0;    // target_type == "owner" 일 때 주문 ID

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedBtnChatBack();
    afx_msg void    OnBnClickedBtnChatSend();
    afx_msg LRESULT OnChatReceived(WPARAM wParam, LPARAM lParam);  // 실시간 메시지
    afx_msg LRESULT OnChatHistory (WPARAM wParam, LPARAM lParam);  // 과거 메시지 일괄

    DECLARE_MESSAGE_MAP()

private:
    void AppendMessage(const ChatMessage& msg);
    void ClearMessages();
};
