//  라이더 핸들러 구현부이다.

#include "RiderHandler.h"
#include "../server/ConnectionManager.h"

//  라이더 요청을 처리한다.
void RiderHandler::handle(const Packet& packet) 
{
    //  예제용 응답을 보낸다.
    ConnectionManager::sendToClient(packet.clientFd, "RIDER_OK|" + packet.payload);
}