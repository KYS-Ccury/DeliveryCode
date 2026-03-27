#pragma once
#include <afxsock.h>
#include "Struct.h" // PacketHeader 사용
#include "json.hpp"

class CClientSocket : public CAsyncSocket
{
public:
    CClientSocket();
    virtual ~CClientSocket();

    bool SendJson(uint16_t protocol, const nlohmann::json& payload);

    // 서버로부터 데이터가 도착했을 때 호출됨
    virtual void OnReceive(int nErrorCode) override;

    // 서버와 연결이 끊겼을 때 호출됨
    virtual void OnClose(int nErrorCode) override;
    // 🚨 연결 및 수신 오버라이드
    virtual void OnConnect(int nErrorCode) override;
    // 서버 접속 명령 함수
    void ConnectForNotify(CString ip, int port);
};