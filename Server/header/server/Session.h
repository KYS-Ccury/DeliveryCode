//  클라이언트 연결 1개를 서버 내부에서 표현하는 세션 객체이다.
//  힙에서 관리하기 쉽게 구조체로 정의한다.

#pragma once

#include <string>
#include <mutex>

//  세션 객체이다.
struct Session {
    //  클라이언트 소켓 번호이다.
    int fd = -1;

    //  로그인한 사용자 ID이다.
    int userId = 0;

    //  로그인 여부이다.
    bool isLoggedIn = false;

    //  수신 버퍼이다.
    std::string readBuffer;

    //  세션 단위 잠금이다.
    std::mutex mtx;
};