// Server/header/server/NetUtils.h

#pragma once

#include <fcntl.h>
#include <unistd.h>

// fd를 non-blocking으로 설정
inline void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}