//  ?뚯빱 ?ㅻ젅?쒕뱾???덉쟾?섍쾶 ?묒뾽??爰쇰궡湲??꾪븳 ?ㅻ젅???덉쟾 ?먯씠??

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

//  ?쒗뵆由?湲곕컲 ?ㅻ젅???덉쟾 ???대옒?ㅼ씠??
template<typename T>
class ThreadSafeQueue {
public:
    //  ?먯뿉 媛믪쓣 ?ｋ뒗??
    void push(const T& value) {
        //  ?좉툑???↔퀬 ?먯뿉 ?곗씠?곕? ?ｋ뒗??
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_queue.push(value);
        }

        //  ?湲?以묒씤 ?ㅻ젅?쒕? 源⑥슫??
        m_cv.notify_one();
    }

    //  ?먯뿉??媛믪쓣 爰쇰궦??
    T pop() {
        //  ?좊땲???쎌쑝濡?議곌굔 蹂?섎? 湲곕떎由곕떎.
        std::unique_lock<std::mutex> lock(m_mtx);

        //  ?먭? 鍮??뚭퉴吏 湲곕떎由곕떎.
        m_cv.wait(lock, [this]() { return !m_queue.empty(); });

        //  留??욎쓽 媛믪쓣 爰쇰궦??
        T value = m_queue.front();
        m_queue.pop();
        return value;
    }

private:
    //  ?ㅼ젣 ?곗씠?곕? ?대뒗 ?먯씠??
    std::queue<T> m_queue;

    //  ??蹂댄샇??裕ㅽ뀓?ㅼ씠??
    std::mutex m_mtx;

    //  ?湲?源⑥슦湲곗슜 議곌굔 蹂?섏씠??
    std::condition_variable m_cv;
};