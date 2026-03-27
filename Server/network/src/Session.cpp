#include "Session.h"
#include "Dispatcher.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <algorithm> // std::min 사용을 위해 추가
#include <ctime>     // 파일명에 시간값을 넣기 위해 추가
#include <nlohmann/json.hpp>

// 일반 JSON 최대 크기 (10MB)
constexpr uint32_t MAX_PACKET_BODY_SIZE = 10 * 1024 * 1024; 
// ★ 파일 수신용 RAM 버퍼 크기 (딱 64KB만 쓴다!)
constexpr size_t FILE_CHUNK_SIZE = 65536; 

// 클라이언트가 파일을 보낼 때 사용할 특수 프로토콜 번호 (예: 9999)
constexpr uint16_t PROTOCOL_FILE_UPLOAD = 9999;

// ★ 이미지 저장 디렉터리 (Customer_Image.cpp의 IMAGE_BASE_DIR과 동일하게 맞출 것)
// 서버 실행 디렉터리 기준. 절대경로 권장: e.g. "/home/lms/bemin/images/"
static const std::string IMAGE_SAVE_DIR = "";  // 빈 문자열 = 실행 디렉터리

Session::Session(int fd) 
    : client_fd(fd), state(State::READING_HEADER), 
      headerBytesRead(0), bodyBytesRead(0) {
    headerBuffer.resize(sizeof(PacketHeader));
}

Session::~Session() {
    // 연결이 비정상적으로 끊겼을 때 열려있는 파일 닫기
    if (m_fileStream.is_open()) m_fileStream.close(); 
    close(client_fd);
}

void Session::resetBuffer() {
    state = State::READING_HEADER;
    headerBytesRead = 0;
    bodyBytesRead = 0;
    headerBuffer.assign(sizeof(PacketHeader), 0);
    bodyBuffer.clear();
    
    if (m_fileStream.is_open()) m_fileStream.close();
}

// ─────────────────────────────────────────────────────────
//  송신 함수 (이전과 완벽히 동일 - Thread Safe 논블로킹)
// ─────────────────────────────────────────────────────────
bool Session::sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody) {
    std::lock_guard<std::mutex> lock(sendMtx); 

    PacketHeader hdr;
    hdr.clientType  = clientType;
    hdr.protocol    = htons(protocol);
    hdr.bodyLength  = htonl(static_cast<uint32_t>(jsonBody.size()));

    std::vector<uint8_t> buf;
    buf.resize(sizeof(PacketHeader) + jsonBody.size());
    std::memcpy(buf.data(), &hdr, sizeof(PacketHeader));
    std::memcpy(buf.data() + sizeof(PacketHeader), jsonBody.data(), jsonBody.size());

    size_t total = buf.size();
    size_t sent  = 0;
    
    while (sent < total) {
        ssize_t ret = send(client_fd,
                           reinterpret_cast<const char*>(buf.data()) + sent,
                           total - sent, MSG_NOSIGNAL);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000); 
                continue;
            }
            std::cerr << "[Session::sendPacket] send 에러 fd=" << client_fd << "\n";
            return false;
        }
        sent += static_cast<size_t>(ret);
    }
    return true;
}

// ─────────────────────────────────────────────────────────
//  ★ 끝판왕 수신 함수 (스트리밍 라우터)
// ─────────────────────────────────────────────────────────
bool Session::readFromSocket(ThreadPool* pool) {
    while (true) {
        // 1. 헤더 읽기
        if (state == State::READING_HEADER) {
            int readLen = recv(client_fd, headerBuffer.data() + headerBytesRead, 
                               sizeof(PacketHeader) - headerBytesRead, 0);
            
            if (readLen == 0) return false; 
            if (readLen < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
                return false;
            }

            headerBytesRead += readLen;
            if (headerBytesRead == sizeof(PacketHeader)) {
                std::memcpy(&currentHeader, headerBuffer.data(), sizeof(PacketHeader));
                currentHeader.protocol   = ntohs(currentHeader.protocol);
                currentHeader.bodyLength = ntohl(currentHeader.bodyLength);

                // ========================================================
                // ★ 라우팅 분기점: 일반 JSON인가? 바이너리 파일인가?
                // ========================================================
                if (currentHeader.protocol == PROTOCOL_FILE_UPLOAD) {
                    // 고유 파일명 생성 (예: user_5_167890123.jpg)
                    m_currentFileName = IMAGE_SAVE_DIR + "upload_" + std::to_string(m_userID) + "_" + std::to_string(time(nullptr)) + ".jpg";
                    
                    // 파일을 추가 쓰기 모드로 연다! (RAM에는 데이터를 담지 않음)
                    m_fileStream.open(m_currentFileName, std::ios::binary | std::ios::app);
                    
                    if (!m_fileStream.is_open()) {
                        std::cerr << "[Session] 파일 스트림 열기 실패\n";
                        return false;
                    }
                    state = State::READING_FILE; // 모드 전환!
                } 
                else {
                    // 일반 JSON 처리 모드
                    if (currentHeader.bodyLength > MAX_PACKET_BODY_SIZE) return false;
                    bodyBuffer.resize(currentHeader.bodyLength);
                    state = State::READING_BODY;
                }
            } else {
                continue;
            }
        }

        // 2. 일반 JSON API 바디 읽기 모드 (기존과 동일)
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

        // 3. ★ 대망의 파일 스트리밍 읽기 모드 (끝판왕 로직)
        if (state == State::READING_FILE) {
            char chunkBuffer[FILE_CHUNK_SIZE]; // RAM은 오직 64KB만 사용!
            
            // 남은 파일 용량과 64KB 중 작은 것을 택해 쪼개어 읽음
            size_t remaining = currentHeader.bodyLength - bodyBytesRead;
            size_t toRead = std::min((size_t)FILE_CHUNK_SIZE, remaining);

            int readLen = recv(client_fd, chunkBuffer, toRead, 0);
            
            if (readLen == 0) return false; // 도중 끊김
            if (readLen < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return true; // 아직 덜 옴, 워커 스레드 놔줌
                return false;
            }

            // 소켓에서 꺼내자마자 RAM에 저장하지 않고 하드디스크로 직행!
            m_fileStream.write(chunkBuffer, readLen);
            bodyBytesRead += readLen;

            // 파일 1개를 전부 다 받았을 때!
            if (bodyBytesRead == currentHeader.bodyLength) {
                m_fileStream.close();
                std::cout << "[FileServer] 성공! 파일 저장 완료: " << m_currentFileName << " (" << bodyBytesRead << " bytes)\n";
                
                // ★ 파일을 무사히 다 받았으니 핸들러에게 "가짜 JSON"을 만들어서 보고함
                // 이렇게 하면 Dispatcher와 핸들러 구조를 1도 안 고치고 그대로 쓸 수 있습니다!
                nlohmann::json fileEvent;
                fileEvent["action"] = "FILE_UPLOAD_SUCCESS";
                fileEvent["file_path"] = m_currentFileName;
                fileEvent["file_size"] = bodyBytesRead;
                
                PacketHeader dummyHeader = currentHeader; // 프로토콜 번호는 9999로 유지
                Dispatcher::dispatch(this, dummyHeader, fileEvent.dump());
                
                resetBuffer(); // 다음 패킷을 위해 초기화
            }
        }
    }
    return true;
}