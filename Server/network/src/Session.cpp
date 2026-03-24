#include "Session.h"
#include "Dispatcher.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <arpa/inet.h>  // htons, htonl, ntohs, ntohl
#include <unistd.h>   // ← 이 줄 추가

Session::Session(int fd) 
    : client_fd(fd), state(State::READING_HEADER), 
      headerBytesRead(0), bodyBytesRead(0) {
    headerBuffer.resize(sizeof(PacketHeader));
}

Session::~Session() {
    close(client_fd);
}

void Session::resetBuffer() {
    state = State::READING_HEADER;
    headerBytesRead = 0;
    bodyBytesRead = 0;
    headerBuffer.assign(sizeof(PacketHeader), 0);
    bodyBuffer.clear();
}

// ─────────────────────────────────────────────────────────
//  클라이언트에게 패킷 전송
//  형식: PacketHeader(1+2+4 바이트, 빅 엔디안) + JSON Body
// ─────────────────────────────────────────────────────────
// bool Session::sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody) {
//     PacketHeader hdr;
//     hdr.clientType  = clientType;
//     hdr.protocol    = htons(protocol);                          // 호스트 → 네트워크 바이트 순서
//     hdr.bodyLength  = htonl(static_cast<uint32_t>(jsonBody.size()));

//     // 헤더 + 바디를 하나의 버퍼에 합쳐서 전송 (TCP 단편화 최소화)
//     std::vector<uint8_t> buf;
//     buf.resize(sizeof(PacketHeader) + jsonBody.size());
//     std::memcpy(buf.data(), &hdr, sizeof(PacketHeader));
//     std::memcpy(buf.data() + sizeof(PacketHeader), jsonBody.data(), jsonBody.size());

//     size_t total = buf.size();
//     size_t sent  = 0;
//     while (sent < total) {
//         ssize_t ret = send(client_fd,
//                            reinterpret_cast<const char*>(buf.data()) + sent,
//                            total - sent, MSG_NOSIGNAL);
//         if (ret <= 0) {
//             std::cerr << "[Session::sendPacket] send 실패 fd=" << client_fd << std::endl;
//             return false;
//         }
//         sent += static_cast<size_t>(ret);
//     }
//     return true;
// }

// ─────────────────────────────────────────────────────────
//  소켓에서 데이터 수신 (epoll 이벤트 발생 시 호출)
//  헤더(7바이트) → 바디(bodyLength 바이트) 순서로 읽음
// ─────────────────────────────────────────────────────────
bool Session::readFromSocket(ThreadPool* pool) {
    while (true) {
        // 1. 헤더 읽기 모드
        if (state == State::READING_HEADER) {
            int readLen = recv(client_fd, headerBuffer.data() + headerBytesRead, 
                               sizeof(PacketHeader) - headerBytesRead, 0);
            
            if (readLen == 0) return false; // 클라이언트 정상 접속 종료
            if (readLen < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
                return false;
            }

            headerBytesRead += readLen;
            if (headerBytesRead == sizeof(PacketHeader)) {
                std::memcpy(&currentHeader, headerBuffer.data(), sizeof(PacketHeader));
                
                // 네트워크 → 호스트 바이트 순서 변환
                currentHeader.protocol   = ntohs(currentHeader.protocol);
                currentHeader.bodyLength = ntohl(currentHeader.bodyLength);

                bodyBuffer.resize(currentHeader.bodyLength);
                state = State::READING_BODY;
            } else {
                continue;
            }
        }

        // 2. 바디 읽기 모드
        if (state == State::READING_BODY) {
            int readLen = 0;
            if (currentHeader.bodyLength > 0) {
                readLen = recv(client_fd, bodyBuffer.data() + bodyBytesRead, 
                               currentHeader.bodyLength - bodyBytesRead, 0);
                
                if (readLen == 0) return false;
                if (readLen < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
                    return false;
                }
                bodyBytesRead += readLen;
            }

            if (bodyBytesRead == currentHeader.bodyLength) {
                std::string bodyData(bodyBuffer.begin(), bodyBuffer.end());
                PacketHeader header = currentHeader;

                Dispatcher::dispatch(this, header, bodyData);
                resetBuffer(); 
            }
        }
    }
    return true;
}
