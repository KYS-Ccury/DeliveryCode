// SocketManager.cpp
// Binary packet protocol matching server Session.cpp
// Format: [clientType:1B][protocol:2B BE][bodyLength:4B BE][JSON body]
#include "pch.h"
#include "SocketManager.h"
#include "Protocol.h"
#include <string>

#pragma comment(lib, "ws2_32.lib")

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

// Binary send: [clientType=3][protocol BE][bodyLen BE][JSON body]
bool SocketManager::SendPacket(UINT16 protocol, const std::string& jsonBody)
{
    if (m_socket == INVALID_SOCKET) return false;

    PacketHeader hdr;
    hdr.clientType = 3;  // RIDER
    hdr.protocol   = htons(protocol);
    hdr.bodyLength = htonl(static_cast<UINT32>(jsonBody.size()));

    // Combine header + body into one send buffer
    std::string buf;
    buf.resize(sizeof(PacketHeader) + jsonBody.size());
    memcpy(&buf[0], &hdr, sizeof(PacketHeader));
    if (!jsonBody.empty())
        memcpy(&buf[sizeof(PacketHeader)], jsonBody.data(), jsonBody.size());

    size_t sent = 0, total = buf.size();
    while (sent < total) {
        int ret = send(m_socket, buf.c_str() + sent,
                       static_cast<int>(total - sent), 0);
        if (ret == SOCKET_ERROR) return false;
        sent += static_cast<size_t>(ret);
    }
    return true;
}

// Legacy wrapper: CString payload -> send as JSON string body
bool SocketManager::SendPacket(int cmd, const CString& payload)
{
    CT2A utf8(payload, CP_UTF8);
    std::string bodyStr(utf8);
    if (!bodyStr.empty() && bodyStr[0] == '{')
        return SendPacket(static_cast<UINT16>(cmd), bodyStr);
    std::string jsonBody = "{\"payload\":\"" + bodyStr + "\"}";
    return SendPacket(static_cast<UINT16>(cmd), jsonBody);
}

UINT SocketManager::RecvThread(LPVOID pParam)
{
    SocketManager* pSelf = static_cast<SocketManager*>(pParam);
    char buf[8192];

    while (pSelf->m_bRunning) {
        int ret = recv(pSelf->m_socket, buf, sizeof(buf), 0);
        if (ret <= 0) {
            if (pSelf->m_hNotifyWnd && IsWindow(pSelf->m_hNotifyWnd))
                PostMessage(pSelf->m_hNotifyWnd, WM_SERVER_DISCONN, 0, 0);
            break;
        }
        pSelf->m_recvBuf.append(buf, ret);
        pSelf->ProcessRecvBuffer();
    }
    return 0;
}

void SocketManager::ProcessRecvBuffer()
{
    if (!m_hNotifyWnd || !IsWindow(m_hNotifyWnd)) return;

    const size_t HEADER_SIZE = sizeof(PacketHeader);  // 7 bytes

    while (m_recvBuf.size() >= HEADER_SIZE) {
        PacketHeader hdr;
        memcpy(&hdr, m_recvBuf.data(), HEADER_SIZE);

        UINT16 protocol   = ntohs(hdr.protocol);
        UINT32 bodyLength = ntohl(hdr.bodyLength);

        if (m_recvBuf.size() < HEADER_SIZE + bodyLength) break;

        std::string body = m_recvBuf.substr(HEADER_SIZE, bodyLength);
        m_recvBuf.erase(0, HEADER_SIZE + bodyLength);

        RecvPacket* pPkt  = new RecvPacket();
        pPkt->protocol    = protocol;
        pPkt->body        = body;

        UINT wmsg = WM_SOCKET_RECV;
        if (protocol == 408) wmsg = WM_DISPATCH_PUSH;
        else if (protocol == 604) wmsg = WM_CHAT_RECV;

        PostMessage(m_hNotifyWnd, wmsg, 0, reinterpret_cast<LPARAM>(pPkt));
    }
}
