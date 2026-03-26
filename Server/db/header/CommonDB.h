#pragma once
#include "MariaDBManager.h"
#include <string>

// ============================================================
//  CommonDB  —  공통 인증/계정 관련 DB 쿼리 모음
//  handlers/Common/src/Basehandler.cpp 의 handleLogin 등에서
//  직접 MariaDB 호출하던 쿼리들을 여기로 이전
// ============================================================
class CommonDB {
public:
    static CommonDB& getInstance() {
        static CommonDB inst;
        return inst;
    }

    // ── 로그인 ───────────────────────────────────────────────
    // login_id + password + role 조건으로 user_id 조회
    // 반환: user_id (없으면 -1)
    int queryLogin(const std::string& loginId,
                   const std::string& password,
                   const std::string& role);

    // ── 회원가입 ─────────────────────────────────────────────
    // users 테이블 INSERT
    // 반환: 새로 생성된 user_id (실패 시 -1)
    int querySignup(const std::string& loginId,
                    const std::string& password,
                    const std::string& role,
                    const std::string& name,
                    const std::string& phone,
                    const std::string& address);

    // ── 프로필 기본 조회 ─────────────────────────────────────
    // user_id → name, phone, address
    struct UserBasic {
        std::string name;
        std::string phone;
        std::string address;
        bool found = false;
    };
    UserBasic queryUserBasic(int userId);

    // ── 비밀번호 확인 & 변경 ─────────────────────────────────
    bool queryCheckPassword(int userId, const std::string& password);
    bool queryChangePassword(int userId, const std::string& newPassword);

    // ── 온라인 상태 변경 (로그인/로그아웃 공통) ──────────────
    // users 테이블의 is_online 필드 (없으면 rider_profiles 에서 처리)
    bool querySetActive(int userId, bool active);

private:
    CommonDB() = default;
    std::string escape(const std::string& s);
};
