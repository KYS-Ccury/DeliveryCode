// ================================================================
//  ChatManager.cpp  ─  실시간 채팅 구현 (NetworkManager 기반)
//
//  [프로토콜]
//  ▶ 채팅방 생성/입장
//    REQ CmdChat::REQ_CREATE_ROOM (600):
//      { "target_id":"store_101", "target_type":"store" }
//    RES: { "status":2000, "room_id":"room_abc123" }
//
//  ▶ 메시지 전송
//    REQ CmdChat::REQ_SEND_MSG (601):
//      { "room_id":"room_abc123", "message":"안녕하세요" }
//    RES: { "status":2000 }
//
//  ▶ 메시지 수신 (서버 PUSH)
//    NTF CmdChat::NTF_RECV_MSG (604):
//      { "room_id":"room_abc123",
//        "sender_id":"store_101",
//        "message":"네, 안녕하세요!",
//        "timestamp":"2024-05-22 12:31" }
//
//  ▶ 과거 메시지 조회
//    REQ CmdChat::REQ_GET_MSGS (602):
//      { "room_id":"room_abc123", "limit":50 }
//    RES: { "status":2000, "messages":[...] }
// ================================================================
#include "pch.h"
#include "ChatManager.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

// WM_CHAT_RECEIVED 는 ChatDlg.h 에 정의
#define WM_CHAT_RECEIVED (WM_USER + 101)

// ── 간이 JSON 헬퍼 ────────────────────────────────────────────
static std::string CMJStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}
static int CMJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); }
    catch (...) { return -1; }
}
static std::string CMEscape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else          out += c;
    }
    return out;
}

// =================================================================

ChatManager::ChatManager()
    : m_hNotifyWnd(nullptr)
{}

bool ChatManager::IsConnected() const
{
    return NetworkManager::GetInstance().IsConnected();
}

// 호환용 (실제 연결은 NetworkManager 에서 이미 완료됨)
bool ChatManager::ConnectToServer(const std::string& ip, int port)
{
    return NetworkManager::GetInstance().IsConnected();
}

void ChatManager::SetCurrentChatPartner(const std::string& partnerID)
{
    m_currentPartnerID = partnerID;
}

// ── 채팅방 생성/입장 ──────────────────────────────────────────
void ChatManager::CreateOrGetRoom(const std::string& targetID,
                                   const std::string& targetType)
{
    m_currentPartnerID = targetID;

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    // REQ_CREATE_ROOM (600)
    std::string json =
        "{\"target_id\":\""   + CMEscape(targetID)   + "\","
        "\"target_type\":\"" + CMEscape(targetType) + "\"}";

    // 방 ID 응답 콜백 (일회성)
    net.RegisterCallback(CmdChat::REQ_CREATE_ROOM,
        [this](uint16_t, const std::string& body) {
            int status = CMJInt(body, "status");
            if (status == (int)Status::SUCCESS)
                m_currentRoomID = CMJStr(body, "room_id");
        });

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdChat::REQ_CREATE_ROOM, json);
}

// ── 수신 콜백 등록 ────────────────────────────────────────────
void ChatManager::RegisterReceiveCallback(HWND hWnd)
{
    m_hNotifyWnd = hWnd;

    // NTF_RECV_MSG (604) 수신 시 ChatDlg 에 PostMessage
    NetworkManager::GetInstance().RegisterCallback(CmdChat::NTF_RECV_MSG,
        [this](uint16_t, const std::string& body) {
            if (!m_hNotifyWnd) return;

            ChatMessage* pMsg = new ChatMessage();
            pMsg->senderID  = CMJStr(body, "sender_id");
            pMsg->message   = CMJStr(body, "message");
            pMsg->timestamp = CMJStr(body, "timestamp");
            pMsg->isMine    = false;

            // 로컬 히스토리에 추가
            {
                std::lock_guard<std::mutex> lock(m_historyMutex);
                m_chatHistory[m_currentPartnerID].push_back(*pMsg);
            }

            ::PostMessage(m_hNotifyWnd, WM_CHAT_RECEIVED, 0, (LPARAM)pMsg);
        });
}

void ChatManager::UnregisterReceiveCallback()
{
    m_hNotifyWnd = nullptr;
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::NTF_RECV_MSG);
}

// ── 메시지 전송 ───────────────────────────────────────────────
bool ChatManager::SendMessageTo(const std::string& targetID,
                                 const std::string& msg)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return false;
    if (m_currentRoomID.empty()) return false;

    std::string json =
        "{\"room_id\":\""  + CMEscape(m_currentRoomID) + "\","
        "\"message\":\""  + CMEscape(msg)              + "\"}";

    // REQ_SEND_MSG (601)
    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdChat::REQ_SEND_MSG, json);

    // 로컬 히스토리에 내 메시지 추가
    ChatMessage myMsg;
    myMsg.senderID = AuthManager::GetInstance().GetCurrentUserID();
    myMsg.message  = msg;
    myMsg.isMine   = true;
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_chatHistory[targetID].push_back(myMsg);
    }
    return true;
}

// ── 과거 메시지 조회 ──────────────────────────────────────────
std::vector<ChatMessage> ChatManager::GetChatHistory(const std::string& targetID)
{
    // 로컬 캐시가 있으면 즉시 반환
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        auto it = m_chatHistory.find(targetID);
        if (it != m_chatHistory.end() && !it->second.empty())
            return it->second;
    }

    // 서버 조회 (REQ_GET_MSGS 602) — 비동기, 결과는 콜백으로 처리
    auto& net = NetworkManager::GetInstance();
    if (net.IsConnected() && !m_currentRoomID.empty()) {
        std::string json =
            "{\"room_id\":\"" + CMEscape(m_currentRoomID) + "\","
            "\"limit\":50}";
        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdChat::REQ_GET_MSGS, json);
        // 응답은 NTF_RECV_MSG 콜백 루트로 처리됨
    }
    return {};
}

void ChatManager::OnMessageReceived(const ChatMessage& msg)
{
    std::lock_guard<std::mutex> lock(m_historyMutex);
    m_chatHistory[m_currentPartnerID].push_back(msg);
}
