#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <memory> // std::make_shared, std::shared_ptr 사용을 위해 필수

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    // 생성자
    ThreadPool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { 
                            return this->stop || !this->tasks.empty(); 
                        });

                        if (this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    
                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "Thread Task Exception: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Thread Task Unknown Exception" << std::endl;
                    }
                }
            });
        }
    }

    // 소멸자 (위치 수정)
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable())
                worker.join();
        }
    }

    // 작업 추가 함수 (위치 수정 및 C++17 문법 반영)
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        // C++17 이상에서는 std::result_of 대신 std::invoke_result_t 를 권장합니다.
        -> std::future<std::invoke_result_t<F, Args...>> {
            
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }
};