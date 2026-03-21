#pragma once
// SocketManager.h
// Binary packet protocol: [clientType:1B][protocol:2B BE][bodyLength:4B BE][JSON body]
// NOTE: pch.h already includes <afxwin.h> <winsock2.h> <ws2tcpip.h> <string> <vector>
//       Do NOT re-include them here.
// IMPORTANT: m_recvBuf uses std::string (not vector<BYTE>) to avoid
//            nlohmann/json picking BYTE as its BinaryType and breaking parse().

#define WM_SOCKET_RECV    (WM_USER + 100)
#define WM_DISPATCH_PUSH  (WM_USER + 101)
#define WM_CHAT_RECV      (WM_USER + 102)
#define WM_SERVER_DISCONN (WM_USER + 103)

// Packet header - matches server Common/header/Packet.h exactly (7 bytes packed)
#pragma pack(push, 1)
struct PacketHeader {
    UINT8  clientType;   // 1 byte : 3 = RIDER
    UINT16 protocol;     // 2 bytes: big-endian
    UINT32 bodyLength;   // 4 bytes: big-endian
};
#pragma pack(pop)

// Completed received packet - passed as lParam via PostMessage.
// Receiver MUST delete after use.
struct RecvPacket {
    UINT16      protocol;
    std::string body;    // UTF-8 JSON
};

class SocketManager {
public:
    SocketManager();
    ~SocketManager();

    bool Connect(const CString& host, int port);
    void Disconnect();

    // Binary send (new style): protocol + UTF-8 JSON body
    bool SendPacket(UINT16 protocol, const std::string& jsonBody = "{}");

    // Legacy wrapper for gradual migration
    bool SendPacket(int cmd, const CString& payload = _T(""));

    bool IsConnected() const { return m_socket != INVALID_SOCKET; }
    void SetNotifyWnd(HWND hWnd) { m_hNotifyWnd = hWnd; }

private:
    static UINT RecvThread(LPVOID pParam);
    void ProcessRecvBuffer();

    SOCKET      m_socket      = INVALID_SOCKET;
    HWND        m_hNotifyWnd  = nullptr;
    CWinThread* m_pRecvThread = nullptr;
    bool        m_bRunning    = false;

    // Use std::string as accumulation buffer (avoids json.hpp picking
    // vector<BYTE> as BinaryType which changes the parser constructor).
    std::string m_recvBuf;
};
