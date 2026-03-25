// SocketManager.cpp
// Binary packet protocol matching server Session.cpp
// Format: [clientType:1B][protocol:2B BE][bodyLength:4B BE][JSON body]
#include "pch.h"
#include "SocketManager.h"
#include "Protocol.h"
#include <string>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

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

void SocketManager::RegisterWnd(UINT16 protocol, HWND hWnd)
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    m_wndMap[protocol] = hWnd;
}

void SocketManager::UnregisterWnd(UINT16 protocol)
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    m_wndMap.erase(protocol);
}

void SocketManager::SetNotifyWnd(HWND hWnd)
{
    m_hFallbackWnd = hWnd;
}

HWND SocketManager::FindWnd(UINT16 protocol)
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    auto it = m_wndMap.find(protocol);
    if (it != m_wndMap.end() && IsWindow(it->second))
        return it->second;
    if (m_hFallbackWnd && IsWindow(m_hFallbackWnd))
        return m_hFallbackWnd;
    return nullptr;
}

bool SocketManager::Connect(const CString& host, int port)
{
    if (m_socket != INVALID_SOCKET) Disconnect();

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    DWORD tvMs = 2000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&tvMs), sizeof(tvMs));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(port));

    CT2A hostA(host);
    if (inet_pton(AF_INET, hostA, &addr.sin_addr) != 1) {
        addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(hostA, nullptr, &hints, &res) == 0 && res) {
            addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
            freeaddrinfo(res);
        } else {
            closesocket(m_socket); m_socket = INVALID_SOCKET; return false;
        }
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_socket); m_socket = INVALID_SOCKET; return false;
    }

    tvMs = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&tvMs), sizeof(tvMs));

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

bool SocketManager::SendPacket(UINT16 protocol, const std::string& jsonBody)
{
    if (m_socket == INVALID_SOCKET) return false;

    PacketHeader hdr;
    hdr.clientType = CLIENT_TYPE_RIDER;
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

bool SocketManager::SendPacket(int cmd, const CString& payload)
{
    CT2A utf8(payload, CP_UTF8);
    std::string bodyStr(utf8);
    if (!bodyStr.empty() && bodyStr[0] == '{')
        return SendPacket(static_cast<UINT16>(cmd), bodyStr);
    return SendPacket(static_cast<UINT16>(cmd),
                      "{\"payload\":\"" + bodyStr + "\"}");
}

UINT SocketManager::RecvThread(LPVOID pParam)
{
    SocketManager* self = static_cast<SocketManager*>(pParam);
    char buf[8192];
    while (self->m_bRunning) {
        int ret = recv(self->m_socket, buf, sizeof(buf), 0);
        if (ret <= 0) {
            HWND hFall = self->m_hFallbackWnd;
            if (hFall && IsWindow(hFall))
                PostMessage(hFall, WM_SERVER_DISCONN, 0, 0);
            break;
        }
        self->m_recvBuf.append(buf, ret);
        self->ProcessRecvBuffer();
    }
    return 0;
}

void SocketManager::ProcessRecvBuffer()
{
    const size_t HDR = sizeof(PacketHeader);
    while (m_recvBuf.size() >= HDR) {
        PacketHeader hdr;
        memcpy(&hdr, m_recvBuf.data(), HDR);
        UINT16 protocol   = ntohs(hdr.protocol);
        UINT32 bodyLength = ntohl(hdr.bodyLength);
        if (m_recvBuf.size() < HDR + bodyLength) break;

        std::string body = m_recvBuf.substr(HDR, bodyLength);
        m_recvBuf.erase(0, HDR + bodyLength);

        RecvPacket* pPkt = new RecvPacket();
        pPkt->protocol   = protocol;
        pPkt->body       = body;

        UINT wmsg = WM_SOCKET_RECV;
        if      (protocol == CMD_RIDER_DISPATCH_PUSH) wmsg = WM_DISPATCH_PUSH;
        else if (protocol == CMD_CHAT_RECV_NTF)       wmsg = WM_CHAT_RECV;

        HWND hTarget = FindWnd(protocol);
        if (hTarget)
            PostMessage(hTarget, wmsg, 0, reinterpret_cast<LPARAM>(pPkt));
        else
            delete pPkt;
    }
}
