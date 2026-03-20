#include "Packet.h"

// ★ 수정됨: 크로스 플랫폼(윈도우<->리눅스) 완벽 호환을 위한 엔디안 변환 적용
bool Session::sendPacket(uint8_t clientType, uint16_t protocol, const std::string& jsonBody) {
    // 1. 보낼 패킷 헤더 조립
    PacketHeader header;
    header.clientType = clientType; // 1바이트는 엔디안 변환이 필요 없음
    
    // ★ Host to Network 변환 (htons: 2바이트 변환, htonl: 4바이트 변환)
    header.protocol = htons(protocol); 
    header.bodyLength = htonl(static_cast<uint32_t>(jsonBody.length()));

    // 2. 전송할 데이터 버퍼 생성 (헤더 + 바디)
    std::vector<uint8_t> sendBuffer(sizeof(PacketHeader) + jsonBody.length());
    
    // 버퍼에 헤더 복사
    std::memcpy(sendBuffer.data(), &header, sizeof(PacketHeader));
    
    // 버퍼에 바디 복사
    if (!jsonBody.empty()) {
        std::memcpy(sendBuffer.data() + sizeof(PacketHeader), jsonBody.data(), jsonBody.length());
    }

    // 3. 소켓으로 전송 (논블로킹 처리)
    size_t totalSent = 0;
    size_t totalLength = sendBuffer.size();

    while (totalSent < totalLength) {
        int sent = send(client_fd, sendBuffer.data() + totalSent, totalLength - totalSent, 0);
        
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // 송신 버퍼 꽉 참, 계속 시도
            }
            std::cerr << "[Session] 데이터 전송 에러 FD: " << client_fd << std::endl;
            return false;
        }
        totalSent += sent;
    }

    return true;
}