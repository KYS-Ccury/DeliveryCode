#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "common/header/Packet.h"
#include "common/header/Types.h"

#pragma comment(lib, "ws2_32.lib")

using PacketCallback = std::function<void(uint16_t protocol, const std::string& jsonBody)>;

class NetworkManager {
public:
    static NetworkManager& GetInstance() {
        static NetworkManager instance;
        return instance;
    }
    bool Connect(const std::string& ip, int port);
    void Disconnect();
    bool IsConnected() const { return m_isConnected.load(); }
    bool SendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody);
    void RegisterCallback(uint16_t protocol, PacketCallback cb);
    void UnregisterCallback(uint16_t protocol);
    void StartReceive();
    void StopReceive();
private:
    NetworkManager();
    ~NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    void ReceiveLoop();
    SOCKET      m_socket = INVALID_SOCKET;
    std::atomic<bool> m_isConnected{ false };
    std::atomic<bool> m_stopReceive{ false };
    std::thread m_recvThread;
    std::mutex  m_sendMutex;
    std::mutex  m_cbMutex;
    std::unordered_map<uint16_t, PacketCallback> m_callbacks;
};
