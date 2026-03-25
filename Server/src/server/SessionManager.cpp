//  ?몄뀡 留ㅻ땲? 援ы쁽遺?대떎.

#include "SessionManager.h"

//  ?몄뀡??異붽??쒕떎.
void SessionManager::addSession(int fd) {
    //  ??μ냼瑜??좉렐??
    std::lock_guard<std::mutex> lock(m_mtx);

    //  ???몄뀡 媛앹껜瑜??숈뿉 ?앹꽦?쒕떎.
    auto session = std::make_shared<Session>(fd);

    //  ??μ냼???깅줉?쒕떎.
    m_sessions[fd] = session;
}

//  ?몄뀡???쒓굅?쒕떎.
void SessionManager::removeSession(int fd) {
    //  ??μ냼瑜??좉렐??
    std::lock_guard<std::mutex> lock(m_mtx);

    //  ?몄뀡????젣?쒕떎.
    m_sessions.erase(fd);
}

//  ?몄뀡??議고쉶?쒕떎.
std::shared_ptr<Session> SessionManager::getSession(int fd) {
    //  ??μ냼瑜??좉렐??
    std::lock_guard<std::mutex> lock(m_mtx);

    //  ?몄뀡??李얜뒗??
    auto it = m_sessions.find(fd);
    if (it == m_sessions.end()) {
        //  ?놁쑝硫?nullptr??諛섑솚?쒕떎.
        return nullptr;
    }

    //  ?몄뀡 怨듭쑀 ?ъ씤?곕? 諛섑솚?쒕떎.
    return it->second;
}