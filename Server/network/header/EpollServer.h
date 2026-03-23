#pragma once

#include <sys/epoll.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <mutex>
#include <memory>

class Session;
class ThreadPool;

class EpollServer {
private:
    static constexpr int MAX_EVENTS = 1024;

    int port;
    int server_fd;
    int epoll_fd;
    ThreadPool* pool;

    std::map<int, std::shared_ptr<Session>> sessions;
    std::mutex session_mutex;

    bool setupServer();
    void setNonBlocking(int fd);
    void acceptConnection();

public:
    EpollServer(int port, ThreadPool* pool);
    ~EpollServer();

    void start();

    void closeConnection(int client_fd);
    void rearmSocket(int client_fd);

    // ★ pushDispatch 에서 사용: fd로 Session 포인터 반환 (없으면 nullptr)
    std::shared_ptr<Session> getSession(int fd);

    // ★ RiderHandler 에서 사용: riderId → fd 매핑은 RiderHandler가 관리,
    //    fd → Session 접근은 이 함수로 처리
    static EpollServer* s_instance; // 싱글턴 포인터 (main에서 설정)
};
