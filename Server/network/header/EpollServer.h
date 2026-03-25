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

    // fd → Session 반환 (없으면 nullptr)
    std::shared_ptr<Session> getSession(int fd);

    // userID → Session 반환 (없으면 nullptr)
    Session* getSessionByUserID(int userID);

    static EpollServer* s_instance;
};
