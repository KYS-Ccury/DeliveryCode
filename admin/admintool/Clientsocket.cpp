#include "pch.h"
#include "ClientSocket.h"
#include <iostream>

CClientSocket::CClientSocket()
    : m_socket(INVALID_SOCKET), m_connected(false), m_wsaInited(false)
{
}

CClientSocket::~CClientSocket()
{
    Disconnect();
    CleanupWinsock();
}

// ==============================
// Winsock 초기화
// ==============================
bool CClientSocket::InitWinsock()
{
    if (m_wsaInited) return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        m_lastError = "WSAStartup 실패";
        return false;
    }

    m_wsaInited = true;
    return true;
}

void CClientSocket::CleanupWinsock()
{
    if (m_wsaInited)
    {
        WSACleanup();
        m_wsaInited = false;
    }
}

// ==============================
// 연결
// ==============================
bool CClientSocket::Connect(const std::string& ip, int port)
{
    if (!InitWinsock()) return false;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET)
    {
        m_lastError = "socket 생성 실패";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        m_lastError = "connect 실패";
        closesocket(m_socket);
        return false;
    }

    m_connected = true;
    return true;
}

void CClientSocket::Disconnect()
{
    if (m_connected)
    {
        closesocket(m_socket);
        m_connected = false;
    }
}

bool CClientSocket::IsConnected() const
{
    return m_connected;
}

// ==============================
// 정확히 N바이트 수신
// ==============================
bool CClientSocket::RecvExact(char* buffer, int size)
{
    int received = 0;

    while (received < size)
    {
        int ret = recv(m_socket, buffer + received, size - received, 0);
        if (ret <= 0)
        {
            m_lastError = "recv 실패";
            return false;
        }
        received += ret;
    }

    return true;
}

// ==============================
// 정확히 N바이트 송신
// ==============================
bool CClientSocket::SendExact(const char* buffer, int size)
{
    int sent = 0;

    while (sent < size)
    {
        int ret = send(m_socket, buffer + sent, size - sent, 0);
        if (ret <= 0)
        {
            m_lastError = "send 실패";
            return false;
        }
        sent += ret;
    }

    return true;
}

// ==============================
// 패킷 생성
// ==============================
std::vector<char> CClientSocket::BuildPacket(uint8_t clientType, uint16_t protocol, const json& body)
{
    std::string bodyStr = body.dump();

    PacketHeader header;
    header.clientType = clientType;
    header.protocol = htons(protocol);
    header.bodyLength = htonl((uint32_t)bodyStr.size());

    std::vector<char> packet(sizeof(PacketHeader) + bodyStr.size());

    memcpy(packet.data(), &header, sizeof(PacketHeader));
    memcpy(packet.data() + sizeof(PacketHeader), bodyStr.data(), bodyStr.size());

    return packet;
}

// ==============================
// 송신
// ==============================
bool CClientSocket::SendPacket(uint8_t clientType, uint16_t protocol, const json& body)
{
    if (!m_connected) return false;

    auto packet = BuildPacket(clientType, protocol, body);
    return SendExact(packet.data(), (int)packet.size());
}

bool CClientSocket::SendAdminPacket(uint16_t protocol, const json& body)
{
    return SendPacket(4, protocol, body); // ADMIN = 4
}

// ==============================
// 수신 (핵심)
// ==============================
RecvResult CClientSocket::RecvPacket()
{
    RecvResult result{};
    result.success = false;

    PacketHeader header;

    // 1. 헤더 먼저 읽기
    if (!RecvExact((char*)&header, sizeof(PacketHeader)))
        return result;

    header.protocol = ntohs(header.protocol);
    header.bodyLength = ntohl(header.bodyLength);

    // 2. 바디 읽기
    std::vector<char> bodyBuf(header.bodyLength);

    if (header.bodyLength > 0)
    {
        if (!RecvExact(bodyBuf.data(), header.bodyLength))
            return result;
    }

    std::string bodyStr(bodyBuf.begin(), bodyBuf.end());

    try
    {
        result.success = true;
        result.header = header;
        result.body = json::parse(bodyStr);
        result.rawBody = bodyStr;
    }
    catch (...)
    {
        m_lastError = "JSON 파싱 실패";
    }

    return result;
}

std::string CClientSocket::GetLastErrorMsg() const
{
    return m_lastError;
}