//?ш린???ㅽ뻾//  ?꾨줈洹몃옩 ?쒖옉???뚯씪?대떎.
//  ?쒕쾭 媛앹껜瑜??앹꽦?섍퀬 ?ы듃瑜??댁뼱 ?대깽??猷⑦봽瑜??쒖옉?쒕떎.

#include <iostream>
#include "server/Server.h"

int main() 
{
    //  ?쒕쾭媛 諛붿씤?⑺븷 ?ы듃 踰덊샇?대떎.
    const int port = 9000;

    try {
        //  ?쒕쾭 媛앹껜瑜??앹꽦?쒕떎.
        Server server(port, 4);

        //  ?쒕쾭瑜??쒖옉?쒕떎.
        server.start();
    }
    catch (const std::exception& e) {
        //  ?덉쇅 諛쒖깮 ??硫붿떆吏瑜?異쒕젰?섍퀬 醫낅즺?쒕떎.
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return 1;
    }

    //  ?뺤긽 醫낅즺 肄붾뱶?대떎.
    return 0;
}