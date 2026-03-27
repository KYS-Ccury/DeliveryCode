#pragma once
// ================================================================
//  ChatManager.h  (고객 채팅 완성 버전)
//
//  [변경 사항]
//    1. room_id 를 int 로 저장 (서버가 숫자로 반환)
//    2. CreateOrGetRoom 에 target_type, order_id 파라미터 추가
//       - target_type = "admin"  : 관리자 채팅  (마이페이지)
//       - target_type = "owner"  : 사장님 채팅  (배달현황)
//    3. REQ_GET_MSGS (602) 응답 콜백 등록 → 과거 메시지 적재
//    4. GetChatHistory 가 서버 조회 결과를 대기 없이 반환하는 구조
//       (서버 응답은 OnHistoryReceived 콜백으로 비동기 수신 →
//        ChatDlg 에 WM_CHAT_HISTORY 메시지로 전달)
//
//  [프로토콜]
//    600  REQ_CREATE_ROOM  →  { "target_type":"admin" }
//                         또는 { "target_type":"owner", "order_id":42 }
//    601  REQ_SEND_MSG     →  { "room_id":7, "message":"..." }
//    602  REQ_GET_MSGS     →  { "room_id":7, "limit":100 }
//    604  NTF_RECV_MSG     ←  서버 Push (실시간 수신)
// ================================================================
#include <string>
#include <vector>
#include <map>
#include <mutex>

#define WM_CHAT_RECEIVED  (WM_USER + 101)  // 실시간 메시지 수신
#define WM_CHAT_HISTORY   (WM_USER + 102)  // 과거 메시지 일괄 수신

struct ChatMessage {
    std::string senderRole;   // "CUSTOMER" | "ADMIN" | "OWNER"
    std::string senderID;
    std::string message;
    std::string timestamp;    // "HH:MM"
    bool        isMine = false;
};

class ChatManager
{
public:
    static ChatManager& GetInstance() {
        static ChatManager instance;
        return instance;
    }

    // ── 채팅방 생성/입장 ──────────────────────────────────────
    // target_type : "admin" | "owner"
    // orderId     : target_type == "owner" 일 때 필수, 그 외 0
    void CreateOrGetRoom(const std::string& targetType, int orderId = 0);

    // ── 수신 콜백 등록/해제 ───────────────────────────────────
    // hWnd 에 WM_CHAT_RECEIVED (실시간) / WM_CHAT_HISTORY (과거) 를 전달
    void RegisterReceiveCallback(HWND hWnd);
    void UnregisterReceiveCallback();

    // ── 메시지 전송 ───────────────────────────────────────────
    bool SendMessage(const std::string& msg);

    // ── 과거 메시지 조회 요청 (비동기) ───────────────────────
    // 즉시 반환, 응답은 WM_CHAT_HISTORY 로 hWnd 에 전달
    void RequestHistory();

    // ── 현재 room_id ─────────────────────────────────────────
    int  GetCurrentRoomId()   const { return m_currentRoomId; }
    bool IsConnected()        const;
    bool IsRoomReady()        const { return m_currentRoomId > 0; }

    // NetworkManager 콜백에서 호출 (내부용)
    void OnMessageReceived(const ChatMessage& msg);

    // 호환용 (no-op)
    bool ConnectToServer(const std::string& ip, int port);
    void Disconnect() {}

    // ── 이전 인터페이스 호환 (ChatDlg 기존 코드 대응) ────────
    // 기존: CreateOrGetRoom(targetID, targetType)
    void CreateOrGetRoom(const std::string& targetID,
                         const std::string& targetType);
    bool SendMessageTo(const std::string& /*targetID*/,
                       const std::string& msg) { return SendMessage(msg); }

    void SetCurrentChatPartner(const std::string& p) { m_partnerLabel = p; }
    std::string GetCurrentRoomID() const { return std::to_string(m_currentRoomId); }
    std::vector<ChatMessage> GetChatHistory(const std::string& /*targetID*/);

private:
    ChatManager();
    ~ChatManager() {}

    HWND        m_hNotifyWnd    = nullptr;
    int         m_currentRoomId = 0;       // 서버에서 받은 실제 room_id (int)
    std::string m_partnerLabel;            // 화면 표시용 ("관리자 문의" 등)
    std::string m_currentTargetType;       // "admin" | "owner"

    std::mutex                                     m_historyMutex;
    std::map<int, std::vector<ChatMessage>>         m_chatHistory; // roomId → 메시지 목록

    void RegisterNtfCallback();   // NTF_RECV_MSG(604) 등록
    void RegisterHistoryCallback(); // REQ_GET_MSGS(602) 응답 등록
};
