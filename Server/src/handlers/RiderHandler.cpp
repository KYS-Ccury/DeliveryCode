//  ?쇱씠???몃뱾??援ы쁽遺?대떎.

#include "RiderHandler.h"
#include "../server/ConnectionManager.h"

//  ?쇱씠???붿껌??泥섎━?쒕떎.
void RiderHandler::handle(const Packet& packet) 
{
    //  ?덉젣???묐떟??蹂대궦??
    ConnectionManager::sendToClient(packet.clientFd, "RIDER_OK|" + packet.payload);
}