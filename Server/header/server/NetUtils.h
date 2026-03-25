// Server/header/server/NetUtils.h

#pragma once

#include <fcntl.h>
#include <unistd.h>

// fd瑜?non-blocking?쇰줈 ?ㅼ젙
inline void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}