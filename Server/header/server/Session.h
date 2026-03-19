// Server/header/server/Session.h

#pragma once

#include <string>
#include <mutex>

// 클라이언트 연결 상태 객체
class Session {
public:
    // 기본 생성자 (SessionManager 호환용)
    Session()
        : fd(-1), isLogin(false), userId(0) {}

    // fd 초기화 생성자
    Session(int fd_)
        : fd(fd_), isLogin(false), userId(0) {}

public:
    int fd;                 // 소켓 FD

    bool isLogin;           // 로그인 여부
    int userId;             // 유저 ID

    std::string readBuffer; // 수신 버퍼

    std::mutex mtx;         // 동기화용
};