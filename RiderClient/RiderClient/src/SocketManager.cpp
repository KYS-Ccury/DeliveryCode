// SocketManager.cpp
// Binary packet protocol matching server Session.cpp
// Format: [clientType:1B][protocol:2B BE][bodyLength:4B BE][JSON body]
#include "pch.h"
#include "SocketManager.h"
#include "Protocol.h"
#include <string>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

// ── 전송용 mutex (UI 스레드 / RecvThread 동시 send 방지) ─────
static std::mutex s_sendMutex;

SocketManager::SocketManager()
{
    WSADATA wsa = {};
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

SocketManager::~SocketManager()
{
    Disconnect();
    WSACleanup();
}

bool SocketManager::Connect(const CString& host, int port)
{
    if (m_socket != INVALID_SOCKET)
        Disconnect();

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    // 연결 타임아웃 2초
    DWORD timeout = 2000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(port));

    CT2A hostA(host);
    if (inet_pton(AF_INET, hostA, &addr.sin_addr) != 1) {
        addrinfo hints = {}, *res = nullptr;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(hostA, nullptr, &hints, &res) == 0 && res) {
            addr.sin_addr =
                reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
            freeaddrinfo(res);
        } else {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    // 연결 성공 후 recv 타임아웃 제거 (블로킹 수신)
    timeout = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    m_bRunning = true;
    m_recvBuf.clear();
    m_pRecvThread = AfxBeginThread(RecvThread, this, THREAD_PRIORITY_NORMAL);
    return true;
}

void SocketManager::Disconnect()
{
    m_bRunning = false;
    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_pRecvThread = nullptr;
    m_recvBuf.clear();
}

// ── 바이너리 전송: [clientType=3][protocol BE][bodyLen BE][body] ─
bool SocketManager::SendPacket(UINT16 protocol, const std::string& jsonBody)
{
    if (m_socket == INVALID_SOCKET) return false;

    PacketHeader hdr;
    hdr.clientType = CLIENT_TYPE_RIDER;   // 3
    hdr.protocol   = htons(protocol);
    hdr.bodyLength = htonl(static_cast<UINT32>(jsonBody.size()));

    std::string buf;
    buf.resize(sizeof(PacketHeader) + jsonBody.size());
    memcpy(&buf[0], &hdr, sizeof(PacketHeader));
    if (!jsonBody.empty())
        memcpy(&buf[sizeof(PacketHeader)], jsonBody.data(), jsonBody.size());

    std::lock_guard<std::mutex> lock(s_sendMutex);
    size_t sent = 0, total = buf.size();
    while (sent < total) {
        int ret = send(m_socket, buf.c_str() + sent,
                       static_cast<int>(total - sent), 0);
        if (ret == SOCKET_ERROR) return false;
        sent += static_cast<size_t>(ret);
    }
    return true;
}

// ── Legacy 래퍼 ──────────────────────────────────────────────────
bool SocketManager::SendPacket(int cmd, const CString& payload)
{
    CT2A utf8(payload, CP_UTF8);
    std::string bodyStr(utf8);
    if (!bodyStr.empty() && bodyStr[0] == '{')
        return SendPacket(static_cast<UINT16>(cmd), bodyStr);
    return SendPacket(static_cast<UINT16>(cmd),
                      "{\"payload\":\"" + bodyStr + "\"}");
}

// ── 수신 스레드 ──────────────────────────────────────────────────
UINT SocketManager::RecvThread(LPVOID pParam)
{
    SocketManager* self = static_cast<SocketManager*>(pParam);
    char buf[8192];

    while (self->m_bRunning) {
        int ret = recv(self->m_socket, buf, sizeof(buf), 0);
        if (ret <= 0) {
            // 서버 연결 끊김
            if (self->m_hNotifyWnd && IsWindow(self->m_hNotifyWnd))
                PostMessage(self->m_hNotifyWnd, WM_SERVER_DISCONN, 0, 0);
            break;
        }
        self->m_recvBuf.append(buf, ret);
        self->ProcessRecvBuffer();
    }
    return 0;
}

// ── 수신 버퍼 처리: 완전한 패킷 단위로 PostMessage ──────────────
void SocketManager::ProcessRecvBuffer()
{
    // NotifyWnd가 유효할 때까지 대기 (LoginDlg 초기화 전 응답 방어)
    if (!m_hNotifyWnd || !IsWindow(m_hNotifyWnd)) return;

    const size_t HDR = sizeof(PacketHeader);  // 7 bytes

    while (m_recvBuf.size() >= HDR) {
        PacketHeader hdr;
        memcpy(&hdr, m_recvBuf.data(), HDR);

        UINT16 protocol   = ntohs(hdr.protocol);
        UINT32 bodyLength = ntohl(hdr.bodyLength);

        // 아직 body가 다 안 왔으면 대기
        if (m_recvBuf.size() < HDR + bodyLength) break;

        std::string body = m_recvBuf.substr(HDR, bodyLength);
        m_recvBuf.erase(0, HDR + bodyLength);

        RecvPacket* pPkt = new RecvPacket();
        pPkt->protocol   = protocol;
        pPkt->body       = body;

        // 프로토콜 번호에 따라 메시지 구분
        UINT wmsg = WM_SOCKET_RECV;
        if      (protocol == CMD_RIDER_DISPATCH_PUSH) wmsg = WM_DISPATCH_PUSH;
        else if (protocol == CMD_CHAT_RECV_NTF)       wmsg = WM_CHAT_RECV;

        // NotifyWnd 재확인 (버퍼 처리 중 창이 닫힐 수 있음)
        if (m_hNotifyWnd && IsWindow(m_hNotifyWnd))
            PostMessage(m_hNotifyWnd, wmsg, 0,
                        reinterpret_cast<LPARAM>(pPkt));
        else
            delete pPkt;
    }
}
