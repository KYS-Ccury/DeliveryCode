#include "pch.h"
#include "NetworkManager.h"

NetworkManager::NetworkManager()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

NetworkManager::~NetworkManager()
{
    Disconnect();
    WSACleanup();
}

bool NetworkManager::Connect(const std::string& ip, int port)
{
    if (m_isConnected.load()) return true;

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    sockaddr_in serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_isConnected.store(true);
    StartReceive();
    return true;
}

void NetworkManager::Disconnect()
{
    if (!m_isConnected.load()) return;
    m_isConnected.store(false);
    StopReceive();
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool NetworkManager::SendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody)
{
    if (!m_isConnected.load() || m_socket == INVALID_SOCKET) return false;
    std::lock_guard<std::mutex> lock(m_sendMutex);

    PacketHeader header;
    header.clientType  = clientType;
    header.protocol    = htons(protocol);
    header.bodyLength  = htonl(static_cast<uint32_t>(jsonBody.size()));

    std::vector<char> buf(sizeof(PacketHeader) + jsonBody.size());
    memcpy(buf.data(), &header, sizeof(PacketHeader));
    if (!jsonBody.empty())
        memcpy(buf.data() + sizeof(PacketHeader), jsonBody.data(), jsonBody.size());

    size_t totalSent = 0;
    while (totalSent < buf.size()) {
        int sent = send(m_socket, buf.data() + totalSent,
                        (int)(buf.size() - totalSent), 0);
        if (sent == SOCKET_ERROR) return false;
        totalSent += sent;
    }
    return true;
}

void NetworkManager::RegisterCallback(uint16_t protocol, PacketCallback cb)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_callbacks[protocol] = std::move(cb);
}

void NetworkManager::UnregisterCallback(uint16_t protocol)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_callbacks.erase(protocol);
}

void NetworkManager::StartReceive()
{
    m_stopReceive.store(false);
    m_recvThread = std::thread(&NetworkManager::ReceiveLoop, this);
}

void NetworkManager::StopReceive()
{
    m_stopReceive.store(true);
    if (m_recvThread.joinable())
        m_recvThread.join();
}

void NetworkManager::ReceiveLoop()
{
    while (!m_stopReceive.load() && m_isConnected.load()) {
        // 1. 헤더 수신 (7바이트)
        PacketHeader header;
        size_t totalRead = 0;
        while (totalRead < sizeof(PacketHeader)) {
            int n = recv(m_socket,
                         reinterpret_cast<char*>(&header) + totalRead,
                         (int)(sizeof(PacketHeader) - totalRead), 0);
            if (n <= 0) { m_isConnected.store(false); return; }
            totalRead += n;
        }
        header.protocol   = ntohs(header.protocol);
        header.bodyLength = ntohl(header.bodyLength);

        // 2. 바디 수신
        std::string body;
        if (header.bodyLength > 0) {
            body.resize(header.bodyLength);
            size_t bodyRead = 0;
            while (bodyRead < header.bodyLength) {
                int n = recv(m_socket,
                             &body[bodyRead],
                             (int)(header.bodyLength - bodyRead), 0);
                if (n <= 0) { m_isConnected.store(false); return; }
                bodyRead += n;
            }
        }

        // 3. 콜백 호출
        PacketCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            auto it = m_callbacks.find(header.protocol);
            if (it != m_callbacks.end()) cb = it->second;
        }
        if (cb) cb(header.protocol, body);
    }
}
