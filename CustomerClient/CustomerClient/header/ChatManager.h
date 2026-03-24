#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>

// ================================================================
//  ChatManager.h  ─  실시간 채팅 관리 (NetworkManager 기반)
//
//  [변경점]
//  - 별도 소켓 없이 기존 NetworkManager TCP 연결 재사용
//  - CmdChat 프로토콜로 전송 (600번대)
//  - RegisterReceiveCallback(HWND) → NTF_RECV_MSG(604) 수신 시
//    PostMessage(hWnd, WM_CHAT_RECEIVED, 0, pMsg)
// ================================================================

struct ChatMessage {
    std::string senderID;
    std::string message;
    std::string timestamp;
    bool        isMine = false;
};

class ChatManager
{
public:
    static ChatManager& GetInstance() {
        static ChatManager instance;
        return instance;
    }

    // 채팅방 생성 또는 기존 방 입장
    void CreateOrGetRoom(const std::string& targetID,
                         const std::string& targetType);

    // WM_CHAT_RECEIVED 수신할 HWND 등록/해제
    void RegisterReceiveCallback(HWND hWnd);
    void UnregisterReceiveCallback();

    // 메시지 전송
    bool SendMessageTo(const std::string& targetID, const std::string& msg);

    // 이전 메시지 내역 (서버 조회 또는 로컬 캐시)
    std::vector<ChatMessage> GetChatHistory(const std::string& targetID);

    // NetworkManager 콜백에서 호출 (내부용)
    void OnMessageReceived(const ChatMessage& msg);

    // 기존 NetworkManager 연결 재사용 → 별도 Connect 불필요
    bool ConnectToServer(const std::string& ip, int port); // 호환용 (no-op)
    void Disconnect() {}
    bool IsConnected() const;

    void SetCurrentChatPartner(const std::string& partnerID);
    std::string GetCurrentRoomID() const { return m_currentRoomID; }

private:
    ChatManager();
    ~ChatManager() {}

    HWND        m_hNotifyWnd   = nullptr;
    std::string m_currentRoomID;
    std::string m_currentPartnerID;

    std::mutex                                     m_historyMutex;
    std::map<std::string, std::vector<ChatMessage>> m_chatHistory;
};
