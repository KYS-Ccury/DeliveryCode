#pragma once
#include "afxdialogex.h"
#include <string>

class CInquiryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CInquiryDlg)
public:
	CInquiryDlg(CWnd* pParent = nullptr);
	virtual ~CInquiryDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INQUIRY_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

public:
	int m_nCurrentRoomId; // 현재 접속 중인 방 번호 (서버 연동용)

	// UI 컨트롤 함수들
	afx_msg void OnBnClickedBtnChatSend();
	afx_msg void OnRadioModeChanged();
	afx_msg void OnDestroy(); // 창 닫힐 때 전역 포인터 해제용

	// 🚨 스레드 대신 CClientSocket이 직접 호출해 줄 수신 함수들
	void OnReceiveChatHistory(const std::string& jsonStr);
	void OnReceiveNewMsg(const std::string& jsonStr);

private:
	void AppendChatToList(const CString& sender, const CString& msg, const CString& time);
};

// CClientSocket이 접근할 수 있도록 전역 포인터 선언
extern CInquiryDlg* g_pInquiryDlg;