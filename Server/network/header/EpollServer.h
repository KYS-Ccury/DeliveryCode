#pragma once

#include <sys/epoll.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <mutex> // 멀티 스레드 환경 보호용

class Session;
class ThreadPool;

class EpollServer {
private:
    static constexpr int MAX_EVENTS = 1024;

    int port;
    int server_fd;
    int epoll_fd;
    ThreadPool* pool;

    std::map<int, Session*> sessions;
    std::mutex session_mutex; // ★ 추가: 세션 맵 보호용 뮤텍스

    bool setupServer();
    void setNonBlocking(int fd);
    void acceptConnection();

public:
    EpollServer(int port, ThreadPool* pool);
    ~EpollServer();

    void start();
    
    // ★ 추가: 워커 스레드가 직접 호출할 수 있도록 public으로 열어둡니다.
    void closeConnection(int client_fd); 
    void rearmSocket(int client_fd);     // EPOLLONESHOT 재활성화
};