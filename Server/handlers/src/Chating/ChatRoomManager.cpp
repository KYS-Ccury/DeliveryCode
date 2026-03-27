// ============================================================
//  ChatRoomManager.cpp
//
//  서브 스레드 1개가 BroadcastTask 큐를 감시하여
//  같은 roomId 에 등록된 모든 참가자에게 실시간 sendPacket.
// ============================================================
#include "ChatRoomManager.h"
#include "EpollServer.h"
#include <iostream>
#include <vector>

// ── 싱글턴 ──────────────────────────────────────────────────
ChatRoomManager& ChatRoomManager::getInstance() {
    static ChatRoomManager inst;
    return inst;
}

ChatRoomManager::ChatRoomManager()  = default;
ChatRoomManager::~ChatRoomManager() { stop(); }

// ============================================================
//  start / stop  —  서버 기동/종료 시 한 번씩 호출
// ============================================================
void ChatRoomManager::start() {
    if (m_running.load()) return;
    m_running.store(true);
    m_worker = std::thread(&ChatRoomManager::workerLoop, this);
    std::cout << "[ChatRoomManager] 서브 스레드 시작" << std::endl;
}

void ChatRoomManager::stop() {
    if (!m_running.load()) return;
    m_running.store(false);
    m_cv.notify_all();
    if (m_worker.joinable())
        m_worker.join();
    std::cout << "[ChatRoomManager] 서브 스레드 종료" << std::endl;
}

// ============================================================
//  joinRoom  —  채팅방에 참가자(fd) 등록
// ============================================================
void ChatRoomManager::joinRoom(int roomId, int fd, ClientType clientType) {
    {
        std::lock_guard<std::mutex> lk(m_roomMtx);
        m_rooms[roomId][fd] = Participant{fd, clientType};
    }
    std::cout << "[ChatRoomManager] joinRoom roomId=" << roomId
              << " fd=" << fd
              << " type=" << static_cast<int>(clientType) << std::endl;
}

// ============================================================
//  leaveRoom  —  채팅방에서 참가자(fd) 제거
// ============================================================
void ChatRoomManager::leaveRoom(int roomId, int fd) {
    std::lock_guard<std::mutex> lk(m_roomMtx);
    auto it = m_rooms.find(roomId);
    if (it == m_rooms.end()) return;

    it->second.erase(fd);
    if (it->second.empty())
        m_rooms.erase(it);

    std::cout << "[ChatRoomManager] leaveRoom roomId=" << roomId
              << " fd=" << fd << std::endl;
}

// ============================================================
//  removeClient  —  연결이 끊어진 fd 를 모든 방에서 일괄 제거
//  EpollServer::closeConnection 에서 호출
// ============================================================
void ChatRoomManager::removeClient(int fd) {
    std::lock_guard<std::mutex> lk(m_roomMtx);
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ) {
        it->second.erase(fd);
        if (it->second.empty())
            it = m_rooms.erase(it);
        else
            ++it;
    }
}

// ============================================================
//  enqueueMessage  —  브로드캐스트 작업을 큐에 삽입 (논블로킹)
//  핸들러(워커풀 스레드)가 호출 → 즉시 반환
//  실제 sendPacket 은 서브 스레드(m_worker)가 처리
// ============================================================
void ChatRoomManager::enqueueMessage(int roomId, int senderFd,
                                     uint16_t protocol,
                                     const std::string& jsonPayload) {
    {
        std::lock_guard<std::mutex> lk(m_taskMtx);
        m_tasks.push(BroadcastTask{roomId, senderFd, protocol, jsonPayload});
    }
    m_cv.notify_one();
}

// ============================================================
//  workerLoop  —  서브 스레드 본체
//  condition_variable 로 큐가 채워질 때까지 대기.
// ============================================================
void ChatRoomManager::workerLoop() {
    std::cout << "[ChatRoomManager] 워커 루프 진입" << std::endl;

    while (true) {
        BroadcastTask task;
        {
            std::unique_lock<std::mutex> lk(m_taskMtx);
            m_cv.wait(lk, [this] {
                return !m_tasks.empty() || !m_running.load();
            });

            if (!m_running.load() && m_tasks.empty()) break;

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        // 락 해제 후 브로드캐스트 실행
        broadcast(task);
    }

    std::cout << "[ChatRoomManager] 워커 루프 종료" << std::endl;
}

// ============================================================
//  broadcast  —  같은 roomId 의 모든 참가자에게 패킷 전송
//
//  ▶ 데드락 방지 설계
//    1단계: m_roomMtx 로 참가자 목록을 vector 에 복사 후 즉시 락 해제
//    2단계: 락 없이 sendPacket 수행
//    3단계: 전송 실패한 fd 만 모아 m_roomMtx 재획득 후 일괄 제거
//    → leaveRoom() 내부도 m_roomMtx 를 잡으므로
//      broadcast() 에서 락을 쥔 채로 leaveRoom() 을 호출하면 데드락.
//      복사 → 락 해제 → 제거 순서로 이를 방지한다.
// ============================================================
void ChatRoomManager::broadcast(const BroadcastTask& task) {
    if (!EpollServer::s_instance) return;

    // ── 1단계: 참가자 목록 복사 (락 최소 구간) ─────────────
    std::vector<Participant> participants;
    {
        std::lock_guard<std::mutex> lk(m_roomMtx);
        auto it = m_rooms.find(task.roomId);
        if (it == m_rooms.end()) return;

        participants.reserve(it->second.size());
        for (const auto& kv : it->second)
            participants.push_back(kv.second);
    }
    // ── 이 시점에서 m_roomMtx 는 해제됨 ────────────────────

    // ── 2단계: 락 없이 sendPacket ──────────────────────────
    std::vector<int> deadFds; // 전송 실패한 fd 수집
    for (const auto& p : participants) {
        auto session = EpollServer::s_instance->getSession(p.fd);
        if (!session) {
            deadFds.push_back(p.fd);
            continue;
        }

        bool ok = session->sendPacket(
            static_cast<uint8_t>(p.clientType),
            task.protocol,
            task.payload);

        if (!ok) {
            std::cerr << "[ChatRoomManager] sendPacket 실패 fd=" << p.fd
                      << " roomId=" << task.roomId << std::endl;
            deadFds.push_back(p.fd);
        }
    }

    // ── 3단계: 실패한 fd 일괄 제거 (재획득) ────────────────
    if (!deadFds.empty()) {
        std::lock_guard<std::mutex> lk(m_roomMtx);
        auto it = m_rooms.find(task.roomId);
        if (it != m_rooms.end()) {
            for (int fd : deadFds)
                it->second.erase(fd);
            if (it->second.empty())
                m_rooms.erase(it);
        }
    }
}
