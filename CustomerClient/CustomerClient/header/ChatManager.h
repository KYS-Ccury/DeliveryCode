#pragma once
#include <string>
#include <vector>

// 실시간 채팅은 보통 소켓(Socket) 통신을 사용하지만, 현재는 프로토타입 단계이므로 
// 메시지 송수신 및 채널 관리 로직 위주로 인터페이스를 설계함.

// 채팅 메시지 구조체
struct ChatMessage {
    std::string senderID;   // 보낸 사람
    std::string message;    // 메시지 내용
    std::string timestamp;  // 보낸 시간
    bool isMine;            // 본인이 보낸 메시지 여부
};

// [COMMON] 실시간 채팅 관리 매니저 (Singleton)
class ChatManager
{
public:
    static ChatManager& GetInstance() {
        static ChatManager instance;
        return instance;
    }

    // --- [연결 및 설정] ---
    // 서버 연결 시도 (IP, Port 등)
    bool ConnectToServer(const std::string& ip, int port);
    void Disconnect();

    // --- [채팅 기능] ---
    // CUS-16: 1:1 채팅 문의 (음식점 또는 관리자 대상)
    bool SendMessageTo(const std::string& targetID, const std::string& msg);

    // 특정 대상과의 대화 내역 가져오기
    std::vector<ChatMessage> GetChatHistory(const std::string& targetID);

    // 메시지 수신 이벤트 핸들러 (UI 업데이트용)
    void OnMessageReceived(const ChatMessage& incomingMsg);

    // --- [상태 관리] ---
    bool IsConnected() const { return m_isConnected; }
    void SetCurrentChatPartner(const std::string& partnerID);

private:
    ChatManager();
    ~ChatManager() {}

    bool m_isConnected;
    std::string m_currentPartnerID; // 현재 채팅 중인 상대
    // 대화 상대별 메시지 리스트 저장 (메모리 내 캐시 예시)
    // std::map<std::string, std::vector<ChatMessage>> m_chatLogs; 
};