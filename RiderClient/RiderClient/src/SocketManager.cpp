#include "pch.h"
#include "SocketManager.h"
#include "Protocol.h"
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// ─────────────────────────────────────────────
// 생성자 / 소멸자
// ─────────────────────────────────────────────
SocketManager::SocketManager()
    : m_socket(INVALID_SOCKET)
    , m_hNotifyWnd(nullptr)
    , m_pRecvThread(nullptr)
    , m_bRunning(false)
{
    WSADATA wsa = {};
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

SocketManager::~SocketManager()
{
    Disconnect();
    WSACleanup();
}

// ─────────────────────────────────────────────
// 서버 연결
// ─────────────────────────────────────────────
bool SocketManager::Connect(const CString& host, int port)
{
    if (m_socket != INVALID_SOCKET)
        Disconnect();

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    // 타임아웃 설정 (연결 2초)
    DWORD timeout = 2000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(port));

    // 호스트 → IP
    CT2A hostA(host);
    if (inet_pton(AF_INET, hostA, &addr.sin_addr) != 1) {
        // 도메인 이름인 경우 getaddrinfo 사용
        addrinfo hints = {}, *res = nullptr;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(hostA, nullptr, &hints, &res) == 0 && res) {
            addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
            freeaddrinfo(res);
        } else {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    // 타임아웃 해제 (이후 수신은 블로킹 스레드에서)
    timeout = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    // 수신 스레드 시작
    m_bRunning    = true;
    m_pRecvThread = AfxBeginThread(RecvThread, this, THREAD_PRIORITY_NORMAL);

    return true;
}

// ─────────────────────────────────────────────
// 연결 해제
// ─────────────────────────────────────────────
void SocketManager::Disconnect()
{
    m_bRunning = false;
    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_pRecvThread = nullptr;
    m_recvBuffer.clear();
}

// ─────────────────────────────────────────────
// 패킷 송신
// 형식: "CMD|payload\n"
// ─────────────────────────────────────────────
bool SocketManager::SendPacket(int cmd, const CString& payload)
{
    if (m_socket == INVALID_SOCKET) return false;

    CString packet;
    if (payload.IsEmpty())
        packet.Format(_T("%d\n"), cmd);
    else
        packet.Format(_T("%d|%s\n"), cmd, static_cast<LPCTSTR>(payload));

    // UTF-8 변환
    CT2A utf8(packet, CP_UTF8);
    std::string data(utf8);

    int total = static_cast<int>(data.size());
    int sent  = 0;
    while (sent < total) {
        int ret = send(m_socket,
                       data.c_str() + sent,
                       total - sent, 0);
        if (ret == SOCKET_ERROR) return false;
        sent += ret;
    }
    return true;
}

// ─────────────────────────────────────────────
// 수신 스레드 (블로킹 recv 루프)
// ─────────────────────────────────────────────
UINT SocketManager::RecvThread(LPVOID pParam)
{
    SocketManager* pSelf = static_cast<SocketManager*>(pParam);
    char buf[4096];

    while (pSelf->m_bRunning) {
        int ret = recv(pSelf->m_socket, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            // 연결 끊김
            if (pSelf->m_hNotifyWnd && IsWindow(pSelf->m_hNotifyWnd))
                PostMessage(pSelf->m_hNotifyWnd, WM_SERVER_DISCONN, 0, 0);
            break;
        }
        buf[ret] = '\0';
        pSelf->m_recvBuffer.append(buf, ret);
        pSelf->ProcessRecvBuffer();
    }
    return 0;
}

// ─────────────────────────────────────────────
// 수신 버퍼 처리 ('\n' 단위로 파싱)
// ─────────────────────────────────────────────
void SocketManager::ProcessRecvBuffer()
{
    if (!m_hNotifyWnd || !IsWindow(m_hNotifyWnd)) return;

    size_t pos;
    while ((pos = m_recvBuffer.find('\n')) != std::string::npos) {
        std::string line = m_recvBuffer.substr(0, pos);
        m_recvBuffer.erase(0, pos + 1);
        if (line.empty()) continue;

        // UTF-8 → CString
        CA2T wline(line.c_str(), CP_UTF8);
        CString msg(wline);

        // CMD 번호 추출
        int pipePos = msg.Find(_T('|'));
        int cmd     = _ttoi(pipePos >= 0 ? msg.Left(pipePos) : msg);

        UINT wmsg = WM_SOCKET_RECV;
        if (cmd == PUSH_DISPATCH)
            wmsg = WM_DISPATCH_PUSH;
        else if (cmd == CMD_CHAT_SEND || cmd == CMD_CHAT_CREATE)
            wmsg = WM_CHAT_RECV;

        // 힙에 복사 → 수신 윈도우에 PostMessage
        // 수신 측에서 delete 해야 함
        CString* pMsg = new CString(msg);
        PostMessage(m_hNotifyWnd, wmsg, 0,
                    reinterpret_cast<LPARAM>(pMsg));
    }
}
