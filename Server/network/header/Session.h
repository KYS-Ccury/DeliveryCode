#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>
#include <fstream>  // ★ 끝판왕 무기: 파일 스트림

#include "Struct.h"
#include "ThreadPool.h"

class Session {
private:
    // ★ READING_FILE 상태 추가!
    enum class State { READING_HEADER, READING_BODY, READING_FILE };

    int      client_fd;
    State    state;
    int      m_userID   = 0;
    uint8_t  m_userType = 0;

    std::vector<uint8_t> headerBuffer;
    std::vector<uint8_t> bodyBuffer;

    size_t       headerBytesRead;
    size_t       bodyBytesRead;
    PacketHeader currentHeader;

    std::mutex   sendMtx;

    // ★ 파일 스트리밍 전용 변수
    std::ofstream m_fileStream;
    std::string   m_currentFileName;

    void resetBuffer();

public:
    Session(int fd);
    ~Session();

    int     getFd()      const { return client_fd; }
    int     getUserID()  const { return m_userID;  }
    uint8_t getUserType()const { return m_userType;}

    void setUserID  (int id)      { m_userID   = id;   }
    void setUserType(uint8_t type){ m_userType = type; }

    bool readFromSocket(ThreadPool* pool);
    bool sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody);
};