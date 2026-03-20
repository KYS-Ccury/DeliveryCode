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
    void dispatchPacket(ThreadPool* pool); // 패킷 완성 시 호출할 헬퍼 함수

public:
    Session(int fd);
    ~Session();

    // 반환값을 bool로 변경: false 반환 시 EpollServer가 연결을 끊음
    bool handleRead(ThreadPool* pool);
    
    // 클라이언트에게 데이터를 보내는 함수 (추가)
    bool sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody);
};