/**
 * ClientSocket.cpp
 * ============================================================
 * ★ 수정사항:
 *   1) AllocConsole / freopen / std::cout 완전 제거
 *   2) LOG 매크로 → _DEBUG 전용 OutputDebugStringA
 *   3) CRITICAL_SECTION 기반 소켓 동기화
 *   4) 폴링 스레드 (heartbeat + 채팅 새메시지 조회)
 *   5) 기존 SendPacket / RecvPacket 로직 100% 유지
 * ============================================================
 */

#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include "ClientSocket.h"
#include <windows.h>
#include <sstream>

 // ============================================================
 // 디버그 로그 매크로
 // ============================================================
#ifdef _DEBUG
#define LOG(msg) \
    { \
        std::ostringstream _oss; \
        _oss << "[ClientSocket] " << msg << "\n"; \
        OutputDebugStringA(_oss.str().c_str()); \
    }
#else
#define LOG(msg)
#endif

// ============================================================
// 생성자 / 소멸자
// ============================================================
CClientSocket::CClientSocket()
    : m_socket(INVALID_SOCKET)
    , m_connected(false)
    , m_wsaInited(false)
    , m_hPollingThread(NULL)
    , m_bPolling(false)
    , m_hNotifyWnd(NULL)
    , m_nPollIntervalMs(5000)
{
    InitializeCriticalSection(&m_cs);
    InitializeCriticalSection(&m_csRoom);
    LOG("CClientSocket 생성");
}

CClientSocket::~CClientSocket()
{
    StopPolling();
    Disconnect();
    CleanupWinsock();
    DeleteCriticalSection(&m_csRoom);
    DeleteCriticalSection(&m_cs);
}

// ============================================================
// 소켓 동기화
// ============================================================
void CClientSocket::Lock()
{
    EnterCriticalSection(&m_cs);
}

void CClientSocket::Unlock()
{
    LeaveCriticalSection(&m_cs);
}

// ============================================================
// Winsock 초기화 / 해제
// ============================================================
bool CClientSocket::InitWinsock()
{
    if (m_wsaInited) return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        m_lastError = "WSAStartup 실패";
        LOG("WSAStartup 실패");
        return false;
    }

    LOG("WSAStartup 성공");
    m_wsaInited = true;
    return true;
}

void CClientSocket::CleanupWinsock()
{
    if (m_wsaInited)
    {
        WSACleanup();
        LOG("WSACleanup 완료");
        m_wsaInited = false;
    }
}

// ============================================================
// 연결
// ============================================================
bool CClientSocket::Connect(const std::string& ip, int port)
{
    LOG("Connect 시도: " << ip << ":" << port);

    if (!InitWinsock()) return false;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET)
    {
        m_lastError = "socket 생성 실패";
        LOG("socket 생성 실패");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        m_lastError = "connect 실패";
        LOG("connect 실패");
        closesocket(m_socket);
        return false;
    }

    LOG("connect 성공");
    m_connected = true;
    return true;
}

void CClientSocket::Disconnect()
{
    StopPolling();

    if (m_connected)
    {
        closesocket(m_socket);
        LOG("소켓 종료");
        m_connected = false;
    }
}

bool CClientSocket::IsConnected() const
{
    return m_connected;
}

// ============================================================
// 정확히 N바이트 수신
// ============================================================
bool CClientSocket::RecvExact(char* buffer, int size)
{
    int received = 0;

    while (received < size)
    {
        int ret = recv(m_socket, buffer + received, size - received, 0);
        if (ret <= 0)
        {
            m_lastError = "recv 실패";
            LOG("recv 실패");
            return false;
        }
        received += ret;
    }

    return true;
}

// ============================================================
// 정확히 N바이트 송신
// ============================================================
bool CClientSocket::SendExact(const char* buffer, int size)
{
    int sent = 0;

    while (sent < size)
    {
        int ret = send(m_socket, buffer + sent, size - sent, 0);
        if (ret <= 0)
        {
            m_lastError = "send 실패";
            LOG("send 실패");
            return false;
        }
        sent += ret;
    }

    return true;
}

// ============================================================
// 패킷 생성
// ============================================================
std::vector<char> CClientSocket::BuildPacket(uint8_t clientType, uint16_t protocol, const json& body)
{
    std::string bodyStr = body.dump();

    PacketHeader header;
    header.clientType = clientType;
    header.protocol = htons(protocol);
    header.bodyLength = htonl((uint32_t)bodyStr.size());

    LOG("패킷 생성");
    LOG("  clientType: " << (int)clientType);
    LOG("  protocol: " << protocol);
    LOG("  bodyLength: " << bodyStr.size());
    LOG("  body: " << bodyStr);

    std::vector<char> packet(sizeof(PacketHeader) + bodyStr.size());
    memcpy(packet.data(), &header, sizeof(PacketHeader));
    memcpy(packet.data() + sizeof(PacketHeader), bodyStr.data(), bodyStr.size());

    return packet;
}

// ============================================================
// 송신
// ============================================================
bool CClientSocket::SendPacket(uint8_t clientType, uint16_t protocol, const json& body)
{
    if (!m_connected)
    {
        LOG("Send 실패: 연결 안됨");
        return false;
    }

    auto packet = BuildPacket(clientType, protocol, body);

    LOG("패킷 송신 시도, size: " << packet.size());

    bool result = SendExact(packet.data(), (int)packet.size());

    if (result)
        LOG("패킷 송신 성공");

    return result;
}

bool CClientSocket::SendAdminPacket(uint16_t protocol, const json& body)
{
    return SendPacket(4, protocol, body);
}

// ============================================================
// 수신
// ============================================================
RecvResult CClientSocket::RecvPacket()
{
    RecvResult result{};
    result.success = false;

    LOG("패킷 수신 대기");

    PacketHeader header;
    if (!RecvExact((char*)&header, sizeof(PacketHeader)))
    {
        LOG("헤더 수신 실패");
        return result;
    }

    header.protocol = ntohs(header.protocol);
    header.bodyLength = ntohl(header.bodyLength);

    LOG("헤더 수신 성공");
    LOG("  protocol: " << header.protocol);
    LOG("  bodyLength: " << header.bodyLength);

    std::vector<char> bodyBuf(header.bodyLength);
    if (header.bodyLength > 0)
    {
        if (!RecvExact(bodyBuf.data(), header.bodyLength))
        {
            LOG("바디 수신 실패");
            return result;
        }
    }

    std::string bodyStr(bodyBuf.begin(), bodyBuf.end());

    LOG("바디 수신 완료");
    LOG("  rawBody: " << bodyStr);

    try
    {
        result.success = true;
        result.header = header;
        result.body = json::parse(bodyStr);
        result.rawBody = bodyStr;

        LOG("JSON 파싱 성공");
    }
    catch (...)
    {
        m_lastError = "JSON 파싱 실패";
        LOG("JSON 파싱 실패");
    }

    return result;
}

// ============================================================
// 마지막 에러 메시지 반환
// ============================================================
std::string CClientSocket::GetLastErrorMsg() const
{
    return m_lastError;
}

// ============================================================
// ★ 폴링 대상 채팅방 ID 설정
// ============================================================
void CClientSocket::SetPollingRoomId(const std::string& roomId)
{
    EnterCriticalSection(&m_csRoom);
    m_strPollingRoomId = roomId;
    LeaveCriticalSection(&m_csRoom);
}

// ============================================================
// ★ 폴링 시작
// ============================================================
void CClientSocket::StartPolling(HWND hNotifyWnd, DWORD nIntervalMs)
{
    if (m_bPolling) return;   // 이미 실행 중

    m_hNotifyWnd = hNotifyWnd;
    m_nPollIntervalMs = nIntervalMs;
    m_bPolling = true;

    m_hPollingThread = CreateThread(
        NULL, 0, PollingThreadProc, this, 0, NULL);

    LOG("폴링 스레드 시작 (주기: " << nIntervalMs << "ms)");
}

// ============================================================
// ★ 폴링 중지
// ============================================================
void CClientSocket::StopPolling()
{
    if (!m_bPolling) return;

    m_bPolling = false;

    if (m_hPollingThread)
    {
        // 최대 (주기 + 2초) 대기 후 강제 종료
        DWORD dwWait = WaitForSingleObject(m_hPollingThread, m_nPollIntervalMs + 2000);
        if (dwWait == WAIT_TIMEOUT)
        {
            LOG("폴링 스레드 강제 종료");
            TerminateThread(m_hPollingThread, 0);
        }
        CloseHandle(m_hPollingThread);
        m_hPollingThread = NULL;
    }

    LOG("폴링 스레드 중지 완료");
}

// ============================================================
// ★ 폴링 스레드 진입점 (static)
// ============================================================
DWORD WINAPI CClientSocket::PollingThreadProc(LPVOID lpParam)
{
    CClientSocket* pThis = static_cast<CClientSocket*>(lpParam);
    pThis->PollingLoop();
    return 0;
}

// ============================================================
// ★ 폴링 루프 (워커 스레드에서 실행)
//
//   매 주기마다:
//     1) Lock → heartbeat 송신/수신 → Unlock
//     2) Lock → 채팅 메시지 조회 송신/수신 → Unlock
//     3) 결과를 PostMessage로 UI에 전달
//
//   Sleep을 짧게 나눠서 m_bPolling 플래그 확인
//   → StopPolling() 호출 시 빠르게 종료
// ============================================================
void CClientSocket::PollingLoop()
{
    LOG("폴링 루프 진입");

    while (m_bPolling && m_connected)
    {
        // --------------------------------------------------------
        // 1) 하트비트
        // --------------------------------------------------------
        {
            Lock();

            bool bHeartbeatOk = false;

            if (m_connected)
            {
                json hbBody;
                if (SendAdminPacket(CMD_HEARTBEAT_REQ, hbBody))
                {
                    RecvResult res = RecvPacket();
                    if (res.success && res.header.protocol == CMD_HEARTBEAT_RES)
                    {
                        bHeartbeatOk = true;
                    }
                }
            }

            Unlock();

            // UI에 결과 통보
            if (::IsWindow(m_hNotifyWnd))
            {
                if (bHeartbeatOk)
                    ::PostMessage(m_hNotifyWnd, WM_POLL_HEARTBEAT_OK, 0, 0);
                else
                    ::PostMessage(m_hNotifyWnd, WM_POLL_HEARTBEAT_FAIL, 0, 0);
            }

            // 하트비트 실패 → 연결 끊김으로 판단, 루프 종료
            if (!bHeartbeatOk)
            {
                LOG("하트비트 실패 → 폴링 종료");
                m_bPolling = false;
                break;
            }
        }

        // --------------------------------------------------------
        // 2) 채팅 새 메시지 조회
        // --------------------------------------------------------
        {
            // 현재 조회 대상 방 ID 복사
            EnterCriticalSection(&m_csRoom);
            std::string roomId = m_strPollingRoomId;
            LeaveCriticalSection(&m_csRoom);

            if (!roomId.empty())
            {
                Lock();

                bool bHasNewMsg = false;

                if (m_connected)
                {
                    json reqBody;
                    reqBody["room_id"] = roomId;

                    if (SendAdminPacket(CMD_GET_MSGS, reqBody))
                    {
                        RecvResult res = RecvPacket();
                        if (res.success
                            && res.body.contains("messages")
                            && res.body["messages"].is_array()
                            && !res.body["messages"].empty())
                        {
                            bHasNewMsg = true;
                        }
                    }
                }

                Unlock();

                if (bHasNewMsg && ::IsWindow(m_hNotifyWnd))
                {
                    ::PostMessage(m_hNotifyWnd, WM_POLL_NEW_MESSAGES, 0, 0);
                }
            }
        }

        // --------------------------------------------------------
        // 3) 인터벌 대기 (100ms 단위로 나눠서 종료 플래그 체크)
        // --------------------------------------------------------
        DWORD dwElapsed = 0;
        while (dwElapsed < m_nPollIntervalMs && m_bPolling)
        {
            Sleep(100);
            dwElapsed += 100;
        }
    }

    LOG("폴링 루프 종료");
}