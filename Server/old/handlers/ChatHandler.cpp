//  채팅 핸들러 구현부이다.

#include "ChatHandler.h"
#include "../server/ConnectionManager.h"

//  채팅 요청을 처리한다.
void ChatHandler::handle(const Packet& packet) {
    //  현재는 간단히 수신 확인 메시지만 보낸다.
    ConnectionManager::sendToClient(packet.clientFd, "CHAT_OK|" + packet.payload);
}