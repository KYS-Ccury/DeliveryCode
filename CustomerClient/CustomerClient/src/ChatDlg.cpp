// ChatDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ChatDlg.h"


// ChatDlg ダイアログ

IMPLEMENT_DYNAMIC(ChatDlg, CDialogEx)

ChatDlg::ChatDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHAT_DLG, pParent)
{

}

ChatDlg::~ChatDlg()
{
}

void ChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ChatDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CHAT_BACK, &ChatDlg::OnBnClickedBtnChatBack)
	ON_BN_CLICKED(IDC_BTN_CHAT_SEND, &ChatDlg::OnBnClickedBtnChatSend)
END_MESSAGE_MAP()


// ChatDlg メッセージ ハンドラー

BOOL ChatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetDlgItemText(IDC_STATIC_CHAT_TITLE, _T("사장님과 채팅"));
	return TRUE;
}

void ChatDlg::OnBnClickedBtnChatBack()
{
	EndDialog(IDCANCEL); // 채팅창 닫기
}

void ChatDlg::OnBnClickedBtnChatSend()
{
    CString strInput;
    GetDlgItemText(IDC_EDIT_CHAT_INPUT, strInput);

    if (!strInput.IsEmpty())
    {
        CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_MSGS);

        // 내 메시지 추가 (예: [나] 안녕하세요)
        CString strMsg;
        strMsg.Format(_T("[나] %s"), strInput);
        int nIdx = pList->AddString(strMsg);

        // 마지막 메시지로 스크롤 이동
        pList->SetCurSel(nIdx);

        // 입력창 비우기
        SetDlgItemText(IDC_EDIT_CHAT_INPUT, _T(""));

        // (참고) 실제 구현 시 여기서 서버로 strInput을 전송해야 합니다.
    }
}