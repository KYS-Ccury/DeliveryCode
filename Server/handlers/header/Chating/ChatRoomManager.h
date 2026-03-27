#pragma once
// ============================================================
//  ChatRoomManager.h
//
//  ▶ 역할
//    채팅방(roomId)마다 참가자 세션(fd)을 관리하고,
//    서브 스레드 1개가 메시지 큐를 감시하여
//    같은 방의 모든 참가자에게 실시간 브로드캐스트한다.
//
//  ▶ 구조
//    map<roomId, set<fd>>          — 방별 참가자 목록
//    queue<BroadcastTask>          — 브로드캐스트 작업 큐
//    thread  m_worker              — 큐를 감시하는 서브 스레드 1개
//
//  ▶ 사용 흐름
//    1. 채팅방 생성/입장  → joinRoom(roomId, fd, clientType)
//    2. 메시지 전송       → enqueueMessage(roomId, senderFd, payload)
//       → 서브 스레드가 같은 방 참가자 전원에게 sendPacket
//    3. 채팅방 퇴장/종료 → leaveRoom(roomId, fd)
// ============================================================

#include "Session.h"
#include "Protocol.h"
#include <map>
#include <set>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cstdint>
#include <atomic>

class ChatRoomManager {
public:
    // ── 싱글턴 ───────────────────────────────────────────────
    static ChatRoomManager& getInstance();

    // ── 참가자 관리 ──────────────────────────────────────────

    // 채팅방에 참가자(fd) 등록. 이미 등록된 경우 덮어쓴다.
    // clientType : 해당 fd 의 ClientType (응답 패킷 헤더에 사용)
    void joinRoom(int roomId, int fd, ClientType clientType);

    // 채팅방에서 참가자(fd) 제거
    void leaveRoom(int roomId, int fd);

    // 연결이 끊어진 fd 를 모든 방에서 일괄 제거
    void removeClient(int fd);

    // ── 메시지 브로드캐스트 ──────────────────────────────────

    // 브로드캐스트 작업을 큐에 넣는다 (논블로킹).
    // 서브 스레드가 이를 꺼내 같은 방 참가자 전원에게 sendPacket.
    // senderFd  : 발신자 fd (자기 자신에게도 전송 — 클라이언트가 에코로 표시)
    // protocol  : CmdChat::NTF_RECV_MSG (604)
    // jsonPayload: 직렬화된 JSON 문자열
    void enqueueMessage(int roomId, int senderFd,
                        uint16_t protocol,
                        const std::string& jsonPayload);

    // ── 라이프사이클 ─────────────────────────────────────────
    void start();   // 서버 기동 시 한 번 호출 → 서브 스레드 시작
    void stop();    // 서버 종료 시 한 번 호출 → 서브 스레드 안전 종료

private:
    ChatRoomManager();
    ~ChatRoomManager();
    ChatRoomManager(const ChatRoomManager&) = delete;
    ChatRoomManager& operator=(const ChatRoomManager&) = delete;

    // ── 참가자 구조체 ─────────────────────────────────────────
    struct Participant {
        int        fd;
        ClientType clientType;
    };

    // ── 브로드캐스트 작업 구조체 ─────────────────────────────
    struct BroadcastTask {
        int         roomId;
        int         senderFd;    // 발신자 (전송 대상에서 제외하지 않음 — 에코 전송)
        uint16_t    protocol;
        std::string payload;
    };

    // ── 방별 참가자 맵  map<roomId, map<fd, Participant>> ────
    // mutex 하나로 rooms_ 와 tasks_ 를 모두 보호
    std::map<int, std::map<int, Participant>> m_rooms;
    std::mutex                                m_roomMtx;

    // ── 브로드캐스트 큐 ──────────────────────────────────────
    std::queue<BroadcastTask>  m_tasks;
    std::mutex                 m_taskMtx;
    std::condition_variable    m_cv;

    // ── 서브 스레드 ──────────────────────────────────────────
    std::thread       m_worker;
    std::atomic<bool> m_running{false};

    // 서브 스레드 루프 — m_cv 를 대기하며 작업을 처리
    void workerLoop();

    // 단일 작업 실행 (워커 스레드 내부 전용)
    void broadcast(const BroadcastTask& task);
};
