//  워커 스레드들이 안전하게 작업을 꺼내기 위한 스레드 안전 큐이다.

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

//  템플릿 기반 스레드 안전 큐 클래스이다.
template<typename T>
class ThreadSafeQueue {
public:
    //  큐에 값을 넣는다.
    void push(const T& value) {
        //  잠금을 잡고 큐에 데이터를 넣는다.
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_queue.push(value);
        }

        //  대기 중인 스레드를 깨운다.
        m_cv.notify_one();
    }

    //  큐에서 값을 꺼낸다.
    T pop() {
        //  유니크 락으로 조건 변수를 기다린다.
        std::unique_lock<std::mutex> lock(m_mtx);

        //  큐가 빌 때까지 기다린다.
        m_cv.wait(lock, [this]() { return !m_queue.empty(); });

        //  맨 앞의 값을 꺼낸다.
        T value = m_queue.front();
        m_queue.pop();
        return value;
    }

private:
    //  실제 데이터를 담는 큐이다.
    std::queue<T> m_queue;

    //  큐 보호용 뮤텍스이다.
    std::mutex m_mtx;

    //  대기/깨우기용 조건 변수이다.
    std::condition_variable m_cv;
};