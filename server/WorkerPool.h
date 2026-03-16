//  라우팅 작업을 별도 스레드에서 처리하기 위한 워커 풀이다.

#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include "ThreadSafeQueue.h"
#include "Packet.h"
#include "Router.h"

class WorkerPool {
public:
    //  생성자이다.
    WorkerPool(int workerCount, Router& router);

    //  소멸자이다.
    ~WorkerPool();

    //  워커 스레드를 시작한다.
    void start();

    //  작업을 큐에 넣는다.
    void enqueue(const Packet& packet);

private:
    //  워커 루프 함수이다.
    void workerLoop();

private:
    //  워커 스레드 개수이다.
    int m_workerCount = 0;

    //  라우터 참조이다.
    Router& m_router;

    //  작업 큐이다.
    ThreadSafeQueue<Packet> m_queue;

    //  워커 스레드들이다.
    std::vector<std::thread> m_threads;

    //  실행 여부 플래그이다.
    std::atomic<bool> m_running { false };
};