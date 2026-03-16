//  워커 풀 구현부이다.

#include "WorkerPool.h"

WorkerPool::WorkerPool(int workerCount, Router& router)
    : m_workerCount(workerCount), m_router(router) {
}

//  소멸자이다.
WorkerPool::~WorkerPool() {
    //  데모 뼈대이므로 강제 join은 생략하지 않고 안전하게 종료 처리한다.
    m_running = false;

    //  대기 중인 스레드를 깨우기 위한 더미 패킷을 넣는다.
    for (int i = 0; i < m_workerCount; ++i) {
        Packet dummy;
        m_queue.push(dummy);
    }

    //  생성된 스레드를 join 한다.
    for (auto& t : m_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

//  워커 스레드를 시작한다.
void WorkerPool::start() {
    //  실행 플래그를 켠다.
    m_running = true;

    //  워커 스레드들을 생성한다.
    for (int i = 0; i < m_workerCount; ++i) {
        m_threads.emplace_back(&WorkerPool::workerLoop, this);
    }
}

//  작업을 큐에 넣는다.
void WorkerPool::enqueue(const Packet& packet) {
    //  패킷을 큐에 넣는다.
    m_queue.push(packet);
}

//  워커 루프이다.
void WorkerPool::workerLoop() {
    while (true) {
        //  큐에서 패킷을 하나 꺼낸다.
        Packet packet = m_queue.pop();

        //  종료 플래그가 꺼져 있으면 빠져나간다.
        if (!m_running) {
            break;
        }

        //  라우터에 패킷 처리를 맡긴다.
        m_router.route(packet);
    }
}