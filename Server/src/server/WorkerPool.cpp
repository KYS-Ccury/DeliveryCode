// Server/src/server/WorkerPool.cpp

#include "WorkerPool.h"
#include "Task.h"

//  생성자
WorkerPool::WorkerPool(int workerCount, Router& router)
    : m_workerCount(workerCount), m_router(router), m_running(false) {
}

//  소멸자
WorkerPool::~WorkerPool() {
    m_running = false;

    //  대기 중인 스레드를 깨우기 위해 더미 Task 삽입
    for (int i = 0; i < m_workerCount; ++i) {
        Task dummy;
        dummy.session = nullptr;
        m_queue.push(dummy);
    }

    //  스레드 종료 대기
    for (auto& t : m_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

//  워커 시작
void WorkerPool::start() {
    m_running = true;

    for (int i = 0; i < m_workerCount; ++i) {
        m_threads.emplace_back(&WorkerPool::workerLoop, this);
    }
}

//  작업 enqueue
void WorkerPool::enqueue(const Task& task) {
    m_queue.push(task);
}

//  워커 루프
void WorkerPool::workerLoop() {
    while (true) {
        Task task = m_queue.pop();

        // 종료 조건
        if (!m_running) {
            break;
        }

        // 더미 task 방어
        if (task.session == nullptr) {
            continue;
        }

        // ⭐ 핵심: session + packet 같이 전달
        m_router.route(task.session, task.packet);
    }
}