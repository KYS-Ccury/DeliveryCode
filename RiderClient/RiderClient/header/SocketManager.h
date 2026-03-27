#pragma once
// SocketManager.h
// Binary packet: [clientType:1B][protocol:2B BE][bodyLength:4B BE][JSON body]
// NOTE: pch.h includes <afxwin.h> <winsock2.h> <ws2tcpip.h>

#define WM_SOCKET_RECV    (WM_USER + 100)
#define WM_DISPATCH_PUSH  (WM_USER + 101)
#define WM_CHAT_RECV      (WM_USER + 102)
#define WM_SERVER_DISCONN (WM_USER + 103)

#include <map>
#include <mutex>

#pragma pack(push, 1)
struct PacketHeader {
    UINT8  clientType;
    UINT16 protocol;
    UINT32 bodyLength;
};
#pragma pack(pop)

// Completed received packet - receiver MUST delete after use.
struct RecvPacket {
    UINT16      protocol;
    std::string body;
};

class SocketManager {
public:
    SocketManager();
    ~SocketManager();

    bool Connect(const CString& host, int port);
    void Disconnect();

    bool SendPacket(UINT16 protocol, const std::string& jsonBody = "{}");
    bool SendPacket(int cmd, const CString& payload = _T(""));

    bool IsConnected() const { return m_socket != INVALID_SOCKET; }

    // -- Window registration API --
    // RegisterWnd  : route incoming protocol to specified HWND
    // UnregisterWnd: call when dialog closes (prevents stale HWND)
    // SetNotifyWnd : fallback window for unregistered protocols
    void RegisterWnd(UINT16 protocol, HWND hWnd);
    void UnregisterWnd(UINT16 protocol);
    void SetNotifyWnd(HWND hWnd);   // fallback for unregistered protocols

private:
    static UINT RecvThread(LPVOID pParam);
    void ProcessRecvBuffer();
    HWND FindWnd(UINT16 protocol);  // internal routing lookup

    SOCKET      m_socket      = INVALID_SOCKET;
    HWND        m_hFallbackWnd = nullptr;       // fallback (SetNotifyWnd)
    CWinThread* m_pRecvThread = nullptr;
    bool        m_bRunning    = false;
    std::string m_recvBuf;

    // protocol -> HWND map (thread-safe)
    std::map<UINT16, HWND> m_wndMap;
    std::mutex             m_wndMutex;
};
