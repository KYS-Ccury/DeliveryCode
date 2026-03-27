#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include "ClientSocket.h"
#include <iostream>
#include <windows.h>

#define LOG(msg) std::cout << "[ClientSocket] " << msg << std::endl;

CClientSocket::CClientSocket()
    : m_socket(INVALID_SOCKET), m_connected(false), m_wsaInited(false)
{
    // ==============================
    // 콘솔 생성 (로그 보기용)
    // ==============================
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);   // ★ 수정 (안전버전)

    std::ios::sync_with_stdio();              // ★ 추가 (버퍼 동기화)

    LOG("콘솔 생성 완료");
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

// ==============================
// 연결
// ==============================
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
            LOG("recv 실패");
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
            LOG("send 실패");
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

    LOG("패킷 생성");
    LOG("clientType: " << (int)clientType);
    LOG("protocol: " << protocol);
    LOG("bodyLength: " << bodyStr.size());
    LOG("body: " << bodyStr);

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

// ==============================
// 수신
// ==============================
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
    LOG("protocol: " << header.protocol);
    LOG("bodyLength: " << header.bodyLength);

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
    LOG("rawBody: " << bodyStr);

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

// ==============================
// 마지막 에러 메시지 반환
// ==============================
std::string CClientSocket::GetLastErrorMsg() const
{
    return m_lastError;
}