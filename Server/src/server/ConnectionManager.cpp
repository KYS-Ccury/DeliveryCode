//  ?곌껐 留ㅻ땲? 援ы쁽遺?대떎.

#include "ConnectionManager.h"
#include <sys/socket.h>
#include <unistd.h>

//  ?뱀젙 ?대씪?댁뼵?몄뿉寃?臾몄옄???묐떟??蹂대궦??
void ConnectionManager::sendToClient(int fd, const std::string& message) {
    //  媛쒗뻾???ы븿?댁꽌 ?대씪?댁뼵?멸? ??以??⑥쐞濡??쎄린 ?쎄쾶 蹂대궦??
    const std::string finalMsg = message + "\n";

    //  send濡??뚯폆???꾩넚?쒕떎.
    ::send(fd, finalMsg.c_str(), finalMsg.size(), 0);
}