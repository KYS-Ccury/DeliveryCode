#include "Session.h"
#include "Dispatcher.h"
#include <iostream>
#include <cstring>
#include <cerrno> // errno, EAGAIN, EWOULDBLOCK 사용
#include <sys/socket.h>
#include <arpa/inet.h>  // ntohs, ntohl 함수 사용을 위해 필수!

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
// ★ 끝판왕 구조: 워커 스레드에서 실행되는 읽기 함수 (엔디안 변환 적용)
bool Session::readFromSocket(ThreadPool* pool) {
    while (true) {
        // 1. 헤더 읽기 모드
        if (state == State::READING_HEADER) {
            int readLen = recv(client_fd, headerBuffer.data() + headerBytesRead, 
                               sizeof(PacketHeader) - headerBytesRead, 0);
            
            if (readLen == 0) return false; // 클라이언트 정상 접속 종료
            if (readLen < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return true; // 다 읽었으니 epoll 재감시(rearm) 요청
                return false; // 실제 네트워크 에러
            }

            headerBytesRead += readLen;
            if (headerBytesRead == sizeof(PacketHeader)) {
                // 버퍼에서 구조체로 메모리 복사
                std::memcpy(&currentHeader, headerBuffer.data(), sizeof(PacketHeader));
                
                // ★ 핵심 추가: 네트워크 바이트 순서(빅 엔디안)를 호스트 바이트 순서(리틀 엔디안)로 복원!
                // 1바이트인 clientType은 변환이 필요 없습니다.
                currentHeader.protocol = ntohs(currentHeader.protocol);       // 2바이트 변환 (Network TO Host Short)
                currentHeader.bodyLength = ntohl(currentHeader.bodyLength);   // 4바이트 변환 (Network TO Host Long)

                // 변환된 정확한 길이를 바탕으로 바디 버퍼 할당
                bodyBuffer.resize(currentHeader.bodyLength);
                state = State::READING_BODY;
            } else {
                continue; // 헤더가 잘려서 왔다면 마저 읽기 위해 루프 계속
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

            // 바디까지 완벽히 1개의 패킷을 다 읽었다면!
            if (bodyBytesRead == currentHeader.bodyLength) {
                std::string bodyData(bodyBuffer.begin(), bodyBuffer.end());
                PacketHeader header = currentHeader;

                // 패킷 처리를 디스패처로 넘김 (나중에 이미지 서버 분리 시 이 부분을 수정하면 됨)
                Dispatcher::dispatch(this, header, bodyData);

                // 다음 패킷이 소켓 버퍼에 연달아 있을 수 있으므로 버퍼를 초기화하고 계속 읽음
                resetBuffer(); 
            }
        }
    }
    return true;
}
