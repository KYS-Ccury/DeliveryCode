#pragma once
#include <afxwin.h>
#include <winsock2.h>
#include <string>

// ─────────────────────────────────────────────
//  커스텀 윈도우 메시지 정의
// ─────────────────────────────────────────────
#define WM_SOCKET_RECV    (WM_USER + 100)   // 일반 서버 응답
#define WM_DISPATCH_PUSH  (WM_USER + 101)   // 신규 배차 Push
#define WM_CHAT_RECV      (WM_USER + 102)   // 채팅 수신
#define WM_SERVER_DISCONN (WM_USER + 103)   // 서버 연결 끊김

// ─────────────────────────────────────────────
//  SocketManager
//  TCP 연결·송신·수신 스레드, WM_USER 메시지 전달
// ─────────────────────────────────────────────
class SocketManager {
public:
    SocketManager();
    ~SocketManager();

    // 서버 연결 (실패해도 앱은 계속 동작)
    bool Connect(const CString& host, int port);
    void Disconnect();

    // "CMD|payload\n" 형식으로 전송
    bool SendPacket(int cmd, const CString& payload = _T(""));

    bool IsConnected() const { return m_socket != INVALID_SOCKET; }

    // 수신 메시지를 PostMessage할 대상 윈도우 설정
    void SetNotifyWnd(HWND hWnd) { m_hNotifyWnd = hWnd; }

private:
    static UINT RecvThread(LPVOID pParam);
    void ProcessRecvBuffer();

    SOCKET      m_socket      = INVALID_SOCKET;
    HWND        m_hNotifyWnd  = nullptr;
    CWinThread* m_pRecvThread = nullptr;
    std::string m_recvBuffer;
    bool        m_bRunning    = false;
};
