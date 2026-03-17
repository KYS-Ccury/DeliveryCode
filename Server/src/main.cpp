//여기서 실행//  프로그램 시작점 파일이다.
//  서버 객체를 생성하고 포트를 열어 이벤트 루프를 시작한다.

#include <iostream>
#include "server/Server.h"

int main() 
{
    //  서버가 바인딩할 포트 번호이다.
    const int port = 9000;

    try {
        //  서버 객체를 생성한다.
        Server server(port, 4);

        //  서버를 시작한다.
        server.start();
    }
    catch (const std::exception& e) {
        //  예외 발생 시 메시지를 출력하고 종료한다.
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return 1;
    }

    //  정상 종료 코드이다.
    return 0;
}