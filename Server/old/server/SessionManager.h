//  세션 객체들을 중앙에서 관리하는 매니저이다.
//  로그인 상태, 연결 상태, 사용자 ID 같은 공통 정보를 관리한다.

#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include "Session.h"

class SessionManager {
public:
    //  세션을 추가한다.
    void addSession(int fd);

    //  세션을 제거한다.
    void removeSession(int fd);

    //  세션을 조회한다.
    std::shared_ptr<Session> getSession(int fd);

private:
    //  fd 기준 세션 저장소이다.
    std::unordered_map<int, std::shared_ptr<Session>> m_sessions;

    //  저장소 보호용 뮤텍스이다.
    std::mutex m_mtx;
};