/**
 * ClientSocket.h
 * ============================================================
 * Winsock 기반 TCP 클라이언트 소켓 클래스
 *
 * 서버 Session::sendPacket()과 대칭되는 구조:
 *   서버 송신: clientType(1) + protocol(2) + bodyLength(4) + JSON
 *   클라 수신: 동일한 순서로 7바이트 헤더 먼저 읽고 → 바디 읽기
 *
 * ★ 수정사항:
 *   1) 폴링 스레드 (heartbeat + 채팅 새메시지 조회)
 *   2) CRITICAL_SECTION 기반 소켓 접근 동기화
 *   3) 폴링 대상 윈도우(HWND) 설정 → PostMessage로 UI 갱신
 * ============================================================
 */

#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

#include "PacketDef.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// ============================================================
// 수신 결과 구조체
// ============================================================
struct RecvResult
{
    bool         success;
    PacketHeader header;
    json         body;
    std::string  rawBody;
};

// ============================================================
// CClientSocket 클래스
// ============================================================
class CClientSocket
{
public:
    CClientSocket();
    ~CClientSocket();

    // --------------------------------------------------------
    // 연결 관리
    // --------------------------------------------------------
    bool Connect(const std::string& ip, int port);
    void Disconnect();
    bool IsConnected() const;

    // --------------------------------------------------------
    // 패킷 송신
    // --------------------------------------------------------
    bool SendAdminPacket(uint16_t protocol, const json& body);
    bool SendPacket(uint8_t clientType, uint16_t protocol, const json& body);

    // --------------------------------------------------------
    // 패킷 수신
    // --------------------------------------------------------
    RecvResult RecvPacket();

    // --------------------------------------------------------
    // 유틸리티
    // --------------------------------------------------------
    std::string GetLastErrorMsg() const;

    // --------------------------------------------------------
    // ★ 소켓 동기화 (폴링 스레드 ↔ UI 스레드 경합 방지)
    //    UI 스레드에서 Send/Recv 할 때 Lock → Unlock
    // --------------------------------------------------------
    void Lock();
    void Unlock();

    // --------------------------------------------------------
    // ★ 폴링 관리
    // --------------------------------------------------------

    /**
     * StartPolling
     * @param hNotifyWnd  폴링 결과를 PostMessage 로 보낼 윈도우 핸들
     * @param nIntervalMs 폴링 주기 (밀리초, 기본 5000ms = 5초)
     *
     * 내부적으로 워커 스레드를 생성하여:
     *   1) CMD_HEARTBEAT_REQ(200) → 서버 응답 확인
     *   2) CMD_GET_MSGS(602) → 현재 선택된 채팅방 새 메시지 확인
     * 결과를 WM_POLL_xxx 메시지로 hNotifyWnd에 전달
     */
    void StartPolling(HWND hNotifyWnd, DWORD nIntervalMs = 5000);

    /**
     * StopPolling
     * 폴링 스레드를 안전하게 종료
     */
    void StopPolling();

    /**
     * SetPollingRoomId
     * 폴링 중 새 메시지를 조회할 채팅방 ID 설정
     * 빈 문자열이면 채팅 메시지 폴링 건너뜀
     */
    void SetPollingRoomId(const std::string& roomId);

private:
    bool RecvExact(char* buffer, int size);
    bool SendExact(const char* buffer, int size);
    std::vector<char> BuildPacket(uint8_t clientType, uint16_t protocol, const json& body);

    bool InitWinsock();
    void CleanupWinsock();

    // ★ 폴링 스레드 함수
    static DWORD WINAPI PollingThreadProc(LPVOID lpParam);
    void PollingLoop();

private:
    SOCKET      m_socket;
    bool        m_connected;
    bool        m_wsaInited;
    std::string m_lastError;

    // ★ 소켓 동기화
    CRITICAL_SECTION m_cs;

    // ★ 폴링 상태
    HANDLE      m_hPollingThread;
    volatile bool m_bPolling;
    HWND        m_hNotifyWnd;
    DWORD       m_nPollIntervalMs;
    std::string m_strPollingRoomId;
    CRITICAL_SECTION m_csRoom;       // m_strPollingRoomId 보호용
};