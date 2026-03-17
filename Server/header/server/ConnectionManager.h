//  연결 종료, 송신 같은 네트워크 연결 관련 공통 유틸 책임을 가진다.

#pragma once

#include <string>

class ConnectionManager {
public:
    //  특정 클라이언트에게 문자열 응답을 보낸다.
    static void sendToClient(int fd, const std::string& message);
};