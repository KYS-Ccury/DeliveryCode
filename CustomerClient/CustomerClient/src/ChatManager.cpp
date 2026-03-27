// ================================================================
//  ChatManager.cpp  (고객 채팅 완성 버전)
//
//  기존 ChatManager.cpp 를 완전히 대체한다.
//
//  주요 변경:
//    - room_id 를 int 로 관리
//    - target_type "admin" / "owner" 로 채팅방 유형 구분
//    - REQ_GET_MSGS(602) 응답을 콜백으로 수신 → WM_CHAT_HISTORY 전달
//    - NTF_RECV_MSG(604) 수신 시 WM_CHAT_RECEIVED 전달
// ================================================================
#include "pch.h"
#include "ChatManager.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

// WM 메시지 정의 (ChatManager.h 에 선언됨)
// #define WM_CHAT_RECEIVED  (WM_USER + 101)
// #define WM_CHAT_HISTORY   (WM_USER + 102)

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

// ================================================================

ChatManager::ChatManager() : m_hNotifyWnd(nullptr), m_currentRoomId(0) {}

bool ChatManager::IsConnected() const
{
    return NetworkManager::GetInstance().IsConnected();
}

bool ChatManager::ConnectToServer(const std::string& /*ip*/, int /*port*/)
{
    return NetworkManager::GetInstance().IsConnected();
}

// ================================================================
//  CreateOrGetRoom  (신규 인터페이스)
//
//  target_type : "admin" | "owner"
//  orderId     : owner 채팅일 때 주문 ID, admin 이면 0
// ================================================================
void ChatManager::CreateOrGetRoom(const std::string& targetType, int orderId)
{
    m_currentTargetType = targetType;
    m_currentRoomId = 0;   // 이전 방 ID 초기화

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    // 혹시 남아있을 이전 600 콜백 먼저 제거
    net.UnregisterCallback(CmdChat::REQ_CREATE_ROOM);

    // ── JSON 요청 구성 ────────────────────────────────────────
    std::string json;
    if (targetType == "owner") {
        json = "{\"target_type\":\"owner\","
               "\"order_id\":"  + std::to_string(orderId) + "}";
    } else {
        // admin (기본)
        json = "{\"target_type\":\"admin\"}";
    }

    // ── 채팅방 생성 응답 콜백 등록 ────────────────────────────
    net.RegisterCallback(CmdChat::REQ_CREATE_ROOM,
        [this](uint16_t, const std::string& body) {
            int status = CMJInt(body, "status");
            if (status == static_cast<int>(Status::SUCCESS)) {
                m_currentRoomId = CMJInt(body, "room_id");

                // NTF / 히스토리 콜백 등록
                RegisterNtfCallback();
                RegisterHistoryCallback();

                // 602 과거메시지 요청
                RequestHistory();

                // ChatDlg 에 방 준비 완료 알림 (WM_CHAT_ROOM_READY)
                if (m_hNotifyWnd)
                    ::PostMessage(m_hNotifyWnd, WM_CHAT_ROOM_READY, 0, 0);
            }
        });

    net.SendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                   CmdChat::REQ_CREATE_ROOM, json);
}

// ================================================================
//  CreateOrGetRoom  (기존 인터페이스 호환)
//  ChatDlg::OnInitDialog 에서 targetType = "admin"/"store" 로 호출함
// ================================================================
void ChatManager::CreateOrGetRoom(const std::string& targetID,
                                   const std::string& targetType)
{
    m_partnerLabel = targetID;

    // 기존 targetType 값 → 새 포맷으로 변환
    std::string newType = (targetType == "store" || targetType == "owner")
                          ? "owner" : "admin";
    // owner 타입이면 order_id 가 별도로 필요하나,
    // 기존 인터페이스에는 orderId 가 없으므로 DeliveryOkDlg 쪽 신규 호출을 권장.
    // 여기서는 admin 처럼 처리하거나, 호출부가 int 버전 인터페이스로 이미 전환됐다면
    // 이 경로는 관리자 채팅(admin)에만 사용된다.
    CreateOrGetRoom(newType, 0);
}

// ================================================================
//  RegisterNtfCallback  —  NTF_RECV_MSG(604) 실시간 수신 등록
// ================================================================
void ChatManager::RegisterNtfCallback()
{
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdChat::NTF_RECV_MSG,
        [this](uint16_t, const std::string& body) {
            if (!m_hNotifyWnd) return;

            ChatMessage* pMsg = new ChatMessage();
            pMsg->senderRole = CMJStr(body, "sender_role");
            pMsg->senderID   = CMJStr(body, "sender_id");
            pMsg->message    = CMJStr(body, "message");
            pMsg->timestamp  = CMJStr(body, "sent_at");

            // sender_role 이 "CUSTOMER" 이면 내가 보낸 메시지
            // (채팅 상대방은 항상 ADMIN 또는 OWNER 이므로 역할로 구분)
            pMsg->isMine = (pMsg->senderRole == "CUSTOMER");

            // 로컬 캐시 추가
            {
                std::lock_guard<std::mutex> lock(m_historyMutex);
                m_chatHistory[m_currentRoomId].push_back(*pMsg);
            }

            ::PostMessage(m_hNotifyWnd, WM_CHAT_RECEIVED, 0, (LPARAM)pMsg);
        });
}

// ================================================================
//  RegisterHistoryCallback  —  REQ_GET_MSGS(602) 응답 등록
// ================================================================
void ChatManager::RegisterHistoryCallback()
{
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdChat::REQ_GET_MSGS,
        [this](uint16_t, const std::string& body) {
            if (!m_hNotifyWnd) return;

            // messages 배열 파싱
            // 형식: {"status":2000,"room_id":7,"messages":[{...},{...}]}
            std::vector<ChatMessage>* pList = new std::vector<ChatMessage>();

            std::string myId = AuthManager::GetInstance().GetCurrentUserID();
            std::string myRole = "CUSTOMER";

            // messages 배열 추출 (간이 파서)
            std::string arrToken = "\"messages\":[";
            auto arrPos = body.find(arrToken);
            if (arrPos != std::string::npos) {
                arrPos += arrToken.size();
                auto arrEnd = body.rfind(']');
                if (arrEnd != std::string::npos && arrEnd > arrPos) {
                    std::string arrStr = body.substr(arrPos, arrEnd - arrPos);
                    // 각 { } 블록 추출
                    size_t p = 0;
                    while (p < arrStr.size()) {
                        auto s = arrStr.find('{', p);
                        if (s == std::string::npos) break;
                        auto e = arrStr.find('}', s);
                        if (e == std::string::npos) break;
                        std::string item = arrStr.substr(s, e - s + 1);

                        ChatMessage msg;
                        msg.senderRole = CMJStr(item, "sender_role");
                        msg.message    = CMJStr(item, "content");
                        msg.timestamp  = CMJStr(item, "sent_at");
                        msg.senderID   = msg.senderRole; // role 을 ID 대체 표시
                        msg.isMine     = (msg.senderRole == myRole);

                        pList->push_back(msg);
                        p = e + 1;
                    }
                }
            }

            // 로컬 캐시 갱신
            {
                std::lock_guard<std::mutex> lock(m_historyMutex);
                m_chatHistory[m_currentRoomId] = *pList;
            }

            // WM_CHAT_HISTORY 로 ChatDlg 에 전달
            ::PostMessage(m_hNotifyWnd, WM_CHAT_HISTORY, 0, (LPARAM)pList);
        });
}

// ================================================================
//  RegisterReceiveCallback  —  ChatDlg 가 열릴 때 HWND 등록
// ================================================================
void ChatManager::RegisterReceiveCallback(HWND hWnd)
{
    m_hNotifyWnd = hWnd;
    // CreateOrGetRoom 이 먼저 호출되고 roomId 가 이미 확정된 경우에는
    // 여기서 바로 콜백 재등록
    if (m_currentRoomId > 0) {
        RegisterNtfCallback();
        RegisterHistoryCallback();
    }
    // roomId 가 아직 0 이면 CreateOrGetRoom 의 응답 콜백 안에서 등록됨
}

void ChatManager::UnregisterReceiveCallback()
{
    m_hNotifyWnd = nullptr;
    // 모든 채팅 관련 콜백 해제 (다음 채팅방 오픈 시 간섭 방지)
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::REQ_CREATE_ROOM);
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::NTF_RECV_MSG);
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::REQ_GET_MSGS);
    m_currentRoomId = 0;
    m_currentTargetType = "";
}

// ================================================================
//  RequestHistory  —  서버에 과거 메시지 100건 조회 요청
// ================================================================
void ChatManager::RequestHistory()
{
    if (m_currentRoomId <= 0) return;
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    std::string json =
        "{\"room_id\":"  + std::to_string(m_currentRoomId) +
        ",\"limit\":100}";

    net.SendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                   CmdChat::REQ_GET_MSGS, json);
}

// ================================================================
//  SendMessage  —  메시지 전송
// ================================================================
bool ChatManager::SendMessage(const std::string& msg)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return false;

    std::string json =
        "{\"room_id\":"  + std::to_string(m_currentRoomId) +
        ",\"message\":\"" + CMEscape(msg) + "\"}";

    net.SendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                   CmdChat::REQ_SEND_MSG, json);
    return true;
}

// ================================================================
//  GetChatHistory  (기존 인터페이스 호환 — 로컬 캐시 반환)
// ================================================================
std::vector<ChatMessage> ChatManager::GetChatHistory(const std::string&)
{
    std::lock_guard<std::mutex> lock(m_historyMutex);
    auto it = m_chatHistory.find(m_currentRoomId);
    if (it != m_chatHistory.end()) return it->second;
    return {};
}

void ChatManager::OnMessageReceived(const ChatMessage& msg)
{
    std::lock_guard<std::mutex> lock(m_historyMutex);
    m_chatHistory[m_currentRoomId].push_back(msg);
}
