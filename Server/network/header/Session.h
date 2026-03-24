#pragma once
#include <vector>
#include <cstdint>
#include "Packet.h"

class ThreadPool;

// Session : 클라이언트 1개 연결에 대응하는 상태 머신
class Session {
public:
    explicit Session(int fd);
    ~Session();

    // 패킷 수신 루프 (EpollServer 워커 스레드에서 호출)
    bool readFromSocket(ThreadPool* pool);

    // 패킷 송신 (서버 → 클라이언트)
    bool sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody);

    // ── 세션 식별자 ──
    int     getFd()      const { return client_fd; }
    int     getUserID()  const { return m_userID;   }
    uint8_t getUserType()const { return m_userType; }

    void setUserID  (int id)      { m_userID   = id;   }
    void setUserType(uint8_t type){ m_userType  = type; }

    void resetBuffer();

private:
    enum class State { READING_HEADER, READING_BODY };

    int     client_fd;
    State   state;
    size_t  headerBytesRead;
    size_t  bodyBytesRead;

    std::vector<uint8_t> headerBuffer;
    std::vector<uint8_t> bodyBuffer;
    PacketHeader         currentHeader;

    // 로그인 후 세팅
    int     m_userID   = 0;
    uint8_t m_userType = 0;
};
