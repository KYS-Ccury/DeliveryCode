// Server/header/server/Session.h

#pragma once

#include <string>
#include <mutex>

// ?대씪?댁뼵???곌껐 ?곹깭 媛앹껜
class Session {
public:
    // 湲곕낯 ?앹꽦??(SessionManager ?명솚??
    Session()
        : fd(-1), isLogin(false), userId(0) {}

    // fd 珥덇린???앹꽦??
    Session(int fd_)
        : fd(fd_), isLogin(false), userId(0) {}

public:
    int fd;                 // ?뚯폆 FD

    bool isLogin;           // 濡쒓렇???щ?
    int userId;             // ?좎? ID

    std::string readBuffer; // ?섏떊 踰꾪띁

    std::mutex mtx;         // ?숆린?붿슜
};