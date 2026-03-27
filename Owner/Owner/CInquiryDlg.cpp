#pragma execution_character_set("utf-8")
#include "pch.h"
#include "Owner.h"
#include "CInquiryDlg.h"
#include "OwnerDlg.h" // 메인 다이얼로그 (m_NotifySocket 접근용)
#include "Protocol.h"
#include "resource.h"
#include "json.hpp"

using json = nlohmann::json;

CInquiryDlg* g_pInquiryDlg = nullptr; // 전역 포인터 초기화

IMPLEMENT_DYNAMIC(CInquiryDlg, CDialogEx)

CInquiryDlg::CInquiryDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_INQUIRY_DIALOG, pParent), m_nCurrentRoomId(0) {
}

CInquiryDlg::~CInquiryDlg() {}

void CInquiryDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CInquiryDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_CHAT_SEND, &CInquiryDlg::OnBnClickedBtnChatSend)
    ON_BN_CLICKED(IDC_RADIO_CENTER, &CInquiryDlg::OnRadioModeChanged)
    ON_BN_CLICKED(IDC_RADIO_CUSTOMER, &CInquiryDlg::OnRadioModeChanged)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CInquiryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 1. 내가 켜졌음을 전역으로 알림 (소켓이 나를 찾을 수 있게!)
    g_pInquiryDlg = this;

    // 2. 고객센터 문의를 기본으로 선택 (사장님 기존 코드 유지)
    CheckRadioButton(IDC_RADIO_CENTER, IDC_RADIO_CUSTOMER, IDC_RADIO_CENTER);
    OnRadioModeChanged(); // 초기 라디오버튼 상태에 따른 서버 접속 로직 실행

    return TRUE;
}

void CInquiryDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
    g_pInquiryDlg = nullptr; // 창 닫히면 포인터 연결 끊기 (소켓이 더 이상 찾지 않음)
}

// =======================================================
// 라디오 버튼(고객센터 / 고객문의) 변경 시 방 다시 접속
// =======================================================
void CInquiryDlg::OnRadioModeChanged()
{
    // 리스트 박스 초기화 (방이 바뀌었으니 화면 지우기)
    CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_HISTORY);
    if (pList) pList->ResetContent();
    m_nCurrentRoomId = 0;

    std::string targetRoomType = "";
    if (IsDlgButtonChecked(IDC_RADIO_CENTER)) {
        targetRoomType = "OWNER_ADMIN"; // 고객센터(관리자)와 대화
    }
    else {
        targetRoomType = "CUSTOMER_OWNER"; // 특정 고객과 대화
    }

    // 서버에 "나 이 방에 들어갈래!" (600번) 요청 쏘기
    json req;
    req["client_type"] = (int)ClientType::OWNER;
    req["room_type"] = targetRoomType;
    req["key"] = 1; // 🚨 임시 주문번호 또는 식별자 (실제 연동 시 동적으로 셋팅)

    COwnerDlg* pMain = (COwnerDlg*)AfxGetMainWnd();
    if (pMain) {
        pMain->m_NotifySocket.SendJson(CmdChat::REQ_CREATE_ROOM, req);
    }
}

// =======================================================
// [송신] 전송 버튼 클릭 시 (601번)
// =======================================================
void CInquiryDlg::OnBnClickedBtnChatSend()
{
    CString strInput;
    GetDlgItemText(IDC_EDIT_CHAT_INPUT, strInput);
    if (strInput.IsEmpty() || m_nCurrentRoomId == 0) return;

    json req;
    req["client_type"] = (int)ClientType::OWNER;
    req["room_id"] = m_nCurrentRoomId;
    req["sender_id"] = 1; // 🚨 사장님 임시 고유 ID
    req["content"] = std::string(CT2CA(strInput, CP_UTF8));

    // 비동기 소켓으로 실제 메시지 쏘기
    COwnerDlg* pMain = (COwnerDlg*)AfxGetMainWnd();
    if (pMain) {
        pMain->m_NotifySocket.SendJson(CmdChat::REQ_SEND_MSG, req);
    }

    SetDlgItemText(IDC_EDIT_CHAT_INPUT, _T("")); // 입력창 비우기
}

// =======================================================
// [수신 1] 방 접속 완료 및 과거 내역 수신 (600번 응답)
// =======================================================
void CInquiryDlg::OnReceiveChatHistory(const std::string& jsonStr)
{
    try {
        json res = json::parse(jsonStr);
        if (res["status"] == Status::SUCCESS) {
            m_nCurrentRoomId = res.value("room_id", 0);

            // 과거 메시지 쫙 뿌려주기
            if (res.contains("messages") && res["messages"].is_array()) {
                for (const auto& msg : res["messages"]) {
                    CString sender = CA2T(msg.value("sender_role", "").c_str(), CP_UTF8);
                    CString content = CA2T(msg.value("content", "").c_str(), CP_UTF8);
                    CString time = CA2T(msg.value("sent_at", "").c_str(), CP_UTF8);
                    AppendChatToList(sender, content, time);
                }
            }
        }
    }
    catch (...) {}
}

// =======================================================
// [수신 2] 실시간 새 메시지 도착 알림 (604번 수신)
// =======================================================
void CInquiryDlg::OnReceiveNewMsg(const std::string& jsonStr)
{
    try {
        json res = json::parse(jsonStr);
        // 현재 내가 켜놓은 방에 온 메시지가 맞는지 확인
        if (res.value("room_id", 0) == m_nCurrentRoomId) {
            CString sender = CA2T(res.value("sender_role", "").c_str(), CP_UTF8);
            CString content = CA2T(res.value("content", "").c_str(), CP_UTF8);
            CString time = CA2T(res.value("sent_at", "").c_str(), CP_UTF8);
            AppendChatToList(sender, content, time);
        }
    }
    catch (...) {}
}

// 리스트박스에 예쁘게 한 줄 추가하는 헬퍼 함수
void CInquiryDlg::AppendChatToList(const CString& sender, const CString& msg, const CString& time)
{
    CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT_HISTORY);
    if (!pList) return;

    // "OWNER"면 "나"로 표시, 아니면 상대방 역할 표시
    CString strSenderName = (sender == _T("OWNER")) ? _T("나") : sender;

    CString strLine;
    strLine.Format(_T("[%s] %s (%s)"), strSenderName, msg, time);

    int nIdx = pList->AddString(strLine);
    pList->SetTopIndex(nIdx); // 스크롤 맨 아래로 자동 이동
}