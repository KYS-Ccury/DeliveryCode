// Server/src/server/WorkerPool.cpp

#include "WorkerPool.h"
#include "Task.h"

//  ?앹꽦??
WorkerPool::WorkerPool(int workerCount, Router& router)
    : m_workerCount(workerCount), m_router(router), m_running(false) {
}

//  ?뚮㈇??
WorkerPool::~WorkerPool() {
    m_running = false;

    //  ?湲?以묒씤 ?ㅻ젅?쒕? 源⑥슦湲??꾪빐 ?붾? Task ?쎌엯
    for (int i = 0; i < m_workerCount; ++i) {
        Task dummy;
        dummy.session = nullptr;
        m_queue.push(dummy);
    }

    //  ?ㅻ젅??醫낅즺 ?湲?
    for (auto& t : m_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

//  ?뚯빱 ?쒖옉
void WorkerPool::start() {
    m_running = true;

    for (int i = 0; i < m_workerCount; ++i) {
        m_threads.emplace_back(&WorkerPool::workerLoop, this);
    }
}

//  ?묒뾽 enqueue
void WorkerPool::enqueue(const Task& task) {
    m_queue.push(task);
}

//  ?뚯빱 猷⑦봽
void WorkerPool::workerLoop() {
    while (true) {
        Task task = m_queue.pop();

        // 醫낅즺 議곌굔
        if (!m_running) {
            break;
        }

        // ?붾? task 諛⑹뼱
        if (task.session == nullptr) {
            continue;
        }

        // 狩??듭떖: session + packet 媛숈씠 ?꾨떖
        m_router.route(task.session, task.packet);
    }
}