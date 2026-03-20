#include "pch.h"
#include "ChatManager.h"

ChatManager::ChatManager()
    : m_isConnected(false)
    , m_currentPartnerID("")
{
}

// 서버 연결 로직
bool ChatManager::ConnectToServer(const std::string& ip, int port)
{
    // TODO: WinSock 등을 이용한 실제 소켓 연결 구현
    m_isConnected = true;
    return true;
}

void ChatManager::Disconnect()
{
    // TODO: 소켓 닫기 처리
    m_isConnected = false;
}

// CUS-16: 메시지 전송
bool ChatManager::SendMessageTo(const std::string& targetID, const std::string& msg)
{
    if (!m_isConnected) return false;

    // TODO: 패킷 구성 후 서버 전송 로직
    // 예: Packet p; p.type = CHAT; p.to = targetID; p.data = msg;

    return true;
}

// 특정인과의 과거 대화 내역 로드
std::vector<ChatMessage> ChatManager::GetChatHistory(const std::string& targetID)
{
    std::vector<ChatMessage> history;
    // TODO: 로컬 DB나 서버에서 해당 유저와의 메시지 기록 로드
    return history;
}

// 서버로부터 메시지를 받았을 때 호출될 함수
void ChatManager::OnMessageReceived(const ChatMessage& incomingMsg)
{
    // TODO: 현재 활성화된 채팅창(ChatDlg)이 있다면 UI 갱신 알림 전송
    // AfxGetMainWnd()->PostMessage(WM_CHAT_RECEIVED, ...);
}

void ChatManager::SetCurrentChatPartner(const std::string& partnerID)
{
    m_currentPartnerID = partnerID;
}