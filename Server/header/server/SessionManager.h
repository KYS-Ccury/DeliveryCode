//  ?몄뀡 媛앹껜?ㅼ쓣 以묒븰?먯꽌 愿由ы븯??留ㅻ땲??대떎.
//  濡쒓렇???곹깭, ?곌껐 ?곹깭, ?ъ슜??ID 媛숈? 怨듯넻 ?뺣낫瑜?愿由ы븳??

#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include "Session.h"

class SessionManager {
public:
    //  ?몄뀡??異붽??쒕떎.
    void addSession(int fd);

    //  ?몄뀡???쒓굅?쒕떎.
    void removeSession(int fd);

    //  ?몄뀡??議고쉶?쒕떎.
    std::shared_ptr<Session> getSession(int fd);

private:
    //  fd 湲곗? ?몄뀡 ??μ냼?대떎.
    std::unordered_map<int, std::shared_ptr<Session>> m_sessions;

    //  ??μ냼 蹂댄샇??裕ㅽ뀓?ㅼ씠??
    std::mutex m_mtx;
};