#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>

#include "Packet.h"      // common 폴더
#include "ThreadPool.h"  // core 폴더

class Session {
private:
    enum class State { READING_HEADER, READING_BODY };
    
    int client_fd;
    State state;
    
    std::vector<uint8_t> headerBuffer; 
    std::vector<uint8_t> bodyBuffer;   
    
    size_t headerBytesRead; 
    size_t bodyBytesRead;   
    PacketHeader currentHeader; 

    void resetBuffer();
    void dispatchPacket(ThreadPool* pool);

public:
    Session(int fd);
    ~Session();

    // ★ 추가: 소켓 fd 반환 (RiderHandler 세션 테이블에서 사용)
    int getFd() const { return client_fd; }

    // 반환값을 bool로 변경: false 반환 시 EpollServer가 연결을 끊음
    bool readFromSocket(ThreadPool* pool);
    bool sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody);
};
