//  梨꾪똿 ?몃뱾??援ы쁽遺?대떎.

#include "ChatHandler.h"
#include "../server/ConnectionManager.h"

//  梨꾪똿 ?붿껌??泥섎━?쒕떎.
void ChatHandler::handle(const Packet& packet) {
    //  ?꾩옱??媛꾨떒???섏떊 ?뺤씤 硫붿떆吏留?蹂대궦??
    ConnectionManager::sendToClient(packet.clientFd, "CHAT_OK|" + packet.payload);
}