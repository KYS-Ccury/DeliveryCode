// Server/header/server/WorkerPool.h

#pragma once
#include <vector>
#include <thread>
#include "ThreadSafeQueue.h"
#include "Task.h"
#include "../router/Router.h"

class WorkerPool {
public:
    WorkerPool(int workerCount, Router& router);
    ~WorkerPool();

    void start();

    //  蹂寃쎈맖
    void enqueue(const Task& task);

private:
    void workerLoop();

private:
    int m_workerCount;
    bool m_running;

    std::vector<std::thread> m_threads;

    // 蹂寃쎈맖
    ThreadSafeQueue<Task> m_queue;

    Router& m_router;
};