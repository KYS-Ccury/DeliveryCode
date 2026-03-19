//  세션 매니저 구현부이다.

#include "SessionManager.h"

//  세션을 추가한다.
void SessionManager::addSession(int fd) {
    //  저장소를 잠근다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  새 세션 객체를 힙에 생성한다.
    auto session = std::make_shared<Session>(fd);

    //  저장소에 등록한다.
    m_sessions[fd] = session;
}

//  세션을 제거한다.
void SessionManager::removeSession(int fd) {
    //  저장소를 잠근다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  세션을 삭제한다.
    m_sessions.erase(fd);
}

//  세션을 조회한다.
std::shared_ptr<Session> SessionManager::getSession(int fd) {
    //  저장소를 잠근다.
    std::lock_guard<std::mutex> lock(m_mtx);

    //  세션을 찾는다.
    auto it = m_sessions.find(fd);
    if (it == m_sessions.end()) {
        //  없으면 nullptr을 반환한다.
        return nullptr;
    }

    //  세션 공유 포인터를 반환한다.
    return it->second;
}