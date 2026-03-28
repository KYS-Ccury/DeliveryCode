// ================================================================
//  ChatManager.cpp  (안정화 최종본)
//
//  [변경 사항]
//    1. 간이 JSON 파서(CMJStr/CMJInt) → nlohmann::json 으로 교체
//       → content 안에 따옴표·중괄호가 있어도 파싱 깨짐 없음
//    2. NTF_RECV_MSG(604) isMine 판정
//       - sender_id 가 내 login_id 와 일치하면 내 메시지
//       - fallback: sender_role == "CUSTOMER"
//    3. owner 채팅에서 orderId <= 0 이면 요청 포기 (방 혼합 방지)
//    4. 기존 호환 오버로드(targetID, targetType)에서
//       owner 타입은 무시 (orderId 없이 방을 만들면 가게별 분리 불가)
//    5. REQ_CREATE_ROOM(600) 응답 콜백 등록 직후
//       NTF / 히스토리 콜백도 즉시 등록
// ================================================================
#include "pch.h"
#include "ChatManager.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"
#include "../header/json.hpp"

using json = nlohmann::json;

// ================================================================
//  생성자
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
//  CreateOrGetRoom  (기본 인터페이스)
//
//  target_type : "admin" | "owner"
//  orderId     : owner 채팅일 때 반드시 > 0
//                admin 채팅은 0 으로 전달
//
//  ※ owner + orderId == 0 → 가게별 채팅방 구분 불가 → 요청 포기
// ================================================================
void ChatManager::CreateOrGetRoom(const std::string& targetType, int orderId)
{
    m_currentTargetType = targetType;
    m_currentRoomId     = 0;   // 이전 방 ID 초기화

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    // 이전 600 콜백 제거
    net.UnregisterCallback(CmdChat::REQ_CREATE_ROOM);

    // ── JSON 요청 구성 ────────────────────────────────────────
    // owner 채팅: orderId 가 없으면 어느 가게 채팅인지 알 수 없으므로 포기
    std::string body;
    if (targetType == "owner") {
        if (orderId <= 0) return;   // 가게별 채팅 분리 불가 → 차단
        body = json{{"target_type","owner"}, {"order_id", orderId}}.dump();
    } else {
        // admin: customer_id 기준 방 → 고객마다 별도 방이 생성됨
        body = json{{"target_type","admin"}}.dump();
    }

    // ── 채팅방 생성/입장 응답 콜백 ───────────────────────────
    net.RegisterCallback(CmdChat::REQ_CREATE_ROOM,
        [this](uint16_t, const std::string& respBody) {
            try {
                json resp     = json::parse(respBody);
                int  status   = resp.value("status", 0);
                if (status != static_cast<int>(Status::SUCCESS)) return;

                m_currentRoomId = resp.value("room_id", 0);

                // NTF / 히스토리 콜백 등록
                RegisterNtfCallback();
                RegisterHistoryCallback();

                // 과거 메시지 요청
                RequestHistory();

                // ChatDlg 에 방 준비 완료 알림
                if (m_hNotifyWnd)
                    ::PostMessage(m_hNotifyWnd, WM_CHAT_ROOM_READY, 0, 0);

            } catch (...) {}
        });

    net.SendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                   CmdChat::REQ_CREATE_ROOM, body);
}

// ================================================================
//  CreateOrGetRoom  (기존 인터페이스 호환)
//  ※ owner 타입은 orderId 없이 방을 만들 수 없으므로 무시
//     admin 채팅에만 사용
// ================================================================
void ChatManager::CreateOrGetRoom(const std::string& targetID,
                                   const std::string& targetType)
{
    m_partnerLabel = targetID;

    std::string newType = (targetType == "store" || targetType == "owner")
                          ? "owner" : "admin";

    if (newType == "owner") {
        // orderId 없이 owner 방 생성 불가 — 무시
        return;
    }

    CreateOrGetRoom("admin", 0);
}

// ================================================================
//  RegisterNtfCallback  —  NTF_RECV_MSG(604) 실시간 수신
// ================================================================
void ChatManager::RegisterNtfCallback()
{
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdChat::NTF_RECV_MSG,
        [this](uint16_t, const std::string& body) {
            if (!m_hNotifyWnd) return;

            try {
                json j = json::parse(body);

                // room_id 필터: 현재 열려 있는 방의 메시지만 처리
                int  msgRoomId  = j.value("room_id", 0);
                if (msgRoomId != m_currentRoomId && m_currentRoomId != 0) return;

                ChatMessage* pMsg = new ChatMessage();
                pMsg->senderRole = j.value("sender_role", "");
                pMsg->senderID   = j.value("sender_id",   "");
                pMsg->message    = j.value("message",     "");
                pMsg->timestamp  = j.value("sent_at",     "");

                // isMine 판정
                // 1순위: sender_id 가 내 login_id 와 일치
                // 2순위: sender_role == "CUSTOMER" (fallback)
                const std::string myId = AuthManager::GetInstance().GetCurrentUserID();
                pMsg->isMine = (!myId.empty() && pMsg->senderID == myId)
                               || (pMsg->senderRole == "CUSTOMER");

                // 로컬 캐시 추가
                {
                    std::lock_guard<std::mutex> lock(m_historyMutex);
                    m_chatHistory[m_currentRoomId].push_back(*pMsg);
                }

                ::PostMessage(m_hNotifyWnd, WM_CHAT_RECEIVED, 0,
                              reinterpret_cast<LPARAM>(pMsg));

            } catch (...) {
                // 파싱 실패 시 조용히 무시
            }
        });
}

// ================================================================
//  RegisterHistoryCallback  —  REQ_GET_MSGS(602) 응답
// ================================================================
void ChatManager::RegisterHistoryCallback()
{
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdChat::REQ_GET_MSGS,
        [this](uint16_t, const std::string& body) {
            if (!m_hNotifyWnd) return;

            std::vector<ChatMessage>* pList = new std::vector<ChatMessage>();

            try {
                json resp = json::parse(body);
                if (resp.value("status", 0) != static_cast<int>(Status::SUCCESS)) {
                    ::PostMessage(m_hNotifyWnd, WM_CHAT_HISTORY, 0,
                                  reinterpret_cast<LPARAM>(pList));
                    return;
                }

                const std::string myRole = "CUSTOMER";
                const std::string myId   = AuthManager::GetInstance().GetCurrentUserID();

                for (const auto& item : resp.value("messages", json::array())) {
                    ChatMessage msg;
                    msg.senderRole = item.value("sender_role", "");
                    msg.message    = item.value("content",     "");
                    msg.timestamp  = item.value("sent_at",     "");
                    msg.senderID   = item.value("sender_id",   msg.senderRole);

                    // isMine 판정 (히스토리)
                    msg.isMine = (!myId.empty() && msg.senderID == myId)
                                 || (msg.senderRole == myRole);

                    pList->push_back(msg);
                }

            } catch (...) {
                // 파싱 실패 시 빈 목록으로 전달
            }

            // 로컬 캐시 갱신
            {
                std::lock_guard<std::mutex> lock(m_historyMutex);
                m_chatHistory[m_currentRoomId] = *pList;
            }

            ::PostMessage(m_hNotifyWnd, WM_CHAT_HISTORY, 0,
                          reinterpret_cast<LPARAM>(pList));
        });
}

// ================================================================
//  RegisterReceiveCallback  —  ChatDlg 가 열릴 때 HWND 등록
// ================================================================
void ChatManager::RegisterReceiveCallback(HWND hWnd)
{
    m_hNotifyWnd = hWnd;
    // 이미 방이 확정된 경우 콜백 재등록
    if (m_currentRoomId > 0) {
        RegisterNtfCallback();
        RegisterHistoryCallback();
    }
}

void ChatManager::UnregisterReceiveCallback()
{
    m_hNotifyWnd = nullptr;
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::REQ_CREATE_ROOM);
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::NTF_RECV_MSG);
    NetworkManager::GetInstance().UnregisterCallback(CmdChat::REQ_GET_MSGS);
    m_currentRoomId     = 0;
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

    std::string body =
        json{{"room_id", m_currentRoomId}, {"limit", 100}}.dump();

    net.SendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                   CmdChat::REQ_GET_MSGS, body);
}

// ================================================================
//  SendMessage  —  메시지 전송
// ================================================================
bool ChatManager::SendMessage(const std::string& msg)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected() || m_currentRoomId <= 0) return false;

    std::string body =
        json{{"room_id", m_currentRoomId}, {"message", msg}}.dump();

    net.SendPacket(static_cast<uint8_t>(ClientType::CUSTOMER),
                   CmdChat::REQ_SEND_MSG, body);
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
