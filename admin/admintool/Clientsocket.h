/**
 * ClientSocket.h
 * ============================================================
 * Winsock 기반 TCP 클라이언트 소켓 클래스
 *
 * 서버 Session::sendPacket()과 대칭되는 구조:
 *   서버 송신: clientType(1) + protocol(2) + bodyLength(4) + JSON
 *   클라 수신: 동일한 순서로 7바이트 헤더 먼저 읽고 → 바디 읽기
 * ============================================================
 */

#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#pragma comment(lib, "ws2_32.lib")  // Winsock 라이브러리 링크

#include "PacketDef.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// ============================================================
// 수신 결과 구조체
// ============================================================
struct RecvResult
{
    bool         success;       // 수신 성공 여부
    PacketHeader header;        // 수신된 패킷 헤더 (7바이트)
    json         body;          // 파싱된 JSON 바디
    std::string  rawBody;       // 원본 JSON 문자열 (디버그용)
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
    bool Connect(const std::string& ip, int port);   // 서버 TCP 연결
    void Disconnect();                                // 연결 해제
    bool IsConnected() const;                         // 연결 상태 확인

    // --------------------------------------------------------
    // 패킷 송신
    // --------------------------------------------------------

    // 관리자 전용 송신 (clientType = 4 고정)
    bool SendAdminPacket(uint16_t protocol, const json& body);

    // 범용 송신
    bool SendPacket(uint8_t clientType, uint16_t protocol, const json& body);

    // --------------------------------------------------------
    // 패킷 수신
    // --------------------------------------------------------

    // 동기 수신: 헤더(7바이트) → 바디(bodyLength) → JSON 파싱
    RecvResult RecvPacket();

    // --------------------------------------------------------
    // 유틸리티
    // --------------------------------------------------------
    std::string GetLastErrorMsg() const;  // 마지막 에러 메시지

private:
    // 정확히 size바이트만큼 수신 (TCP 스트림 대응)
    bool RecvExact(char* buffer, int size);

    // 정확히 size바이트만큼 송신
    bool SendExact(const char* buffer, int size);

    // 패킷 직렬화: Header(7) + JSON → 바이트 배열
    std::vector<char> BuildPacket(uint8_t clientType, uint16_t protocol, const json& body);

    bool InitWinsock();       // Winsock 초기화
    void CleanupWinsock();    // Winsock 정리

private:
    SOCKET      m_socket;       // 소켓 핸들
    bool        m_connected;    // 연결 상태 플래그
    bool        m_wsaInited;    // WSA 초기화 여부
    std::string m_lastError;    // 마지막 에러 메시지
};