//  연결 매니저 구현부이다.

#include "ConnectionManager.h"
#include <sys/socket.h>
#include <unistd.h>

//  특정 클라이언트에게 문자열 응답을 보낸다.
void ConnectionManager::sendToClient(int fd, const std::string& message) {
    //  개행을 포함해서 클라이언트가 한 줄 단위로 읽기 쉽게 보낸다.
    const std::string finalMsg = message + "\n";

    //  send로 소켓에 전송한다.
    ::send(fd, finalMsg.c_str(), finalMsg.size(), 0);
}