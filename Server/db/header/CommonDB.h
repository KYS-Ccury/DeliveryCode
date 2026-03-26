#pragma once
#include "MariaDBManager.h"
#include <string>
#include <vector>

// ============================================================
//  CommonDB  —  전 역할(고객/사장/라이더/관리자) 공통 DB 쿼리
//
//  대상 테이블:
//    users            (계정 공통)
//    payment_methods  (결제수단 - 고객/사장 공통)
//    point_log        (포인트 이력 - 고객 공통)
//
//  사용처:
//    Basehandler::handleLogin    → queryLogin
//    Basehandler::handleSignup   → queryCheckDuplicateId, queryInsertUser
//    Basehandler::handleGetProfile → queryPaymentMethods, queryAddCard 등
//    각 역할별 onLoginSuccess    → queryUserBasic
//    각 역할별 onGetProfile      → queryCheckPassword, queryChangePassword
// ============================================================
class CommonDB {
public:
    static CommonDB& getInstance() {
        static CommonDB inst;
        return inst;
    }

    // ─────────────────────────────────────────────────────────
    //  [로그인] Basehandler::handleLogin 에서 사용
    // ─────────────────────────────────────────────────────────

    // users 테이블에서 login_id + password + role 로 user_id 조회
    // 반환: user_id (없으면 -1)
    int queryLogin(const std::string& loginId,
                   const std::string& password,
                   const std::string& role);

    // ─────────────────────────────────────────────────────────
    //  [회원가입] Basehandler::handleSignup 에서 사용
    // ─────────────────────────────────────────────────────────

    // 아이디 중복 확인 (true = 사용 가능)
    bool queryCheckDuplicateId(const std::string& loginId);

    // users 테이블 INSERT
    // 반환: 새로 생성된 user_id (실패 시 -1)
    int queryInsertUser(const std::string& loginId,
                        const std::string& password,
                        const std::string& role,
                        const std::string& name,
                        const std::string& phone,
                        const std::string& address);

    // ─────────────────────────────────────────────────────────
    //  [기본 정보 조회] 각 역할 onLoginSuccess / onGetProfile 에서 사용
    // ─────────────────────────────────────────────────────────

    struct UserBasic {
        std::string name;
        std::string phone;
        std::string address;
        bool        found = false;
    };

    // user_id → name, phone, address
    UserBasic queryUserBasic(int userId);

    // ─────────────────────────────────────────────────────────
    //  [비밀번호] 각 역할 onGetProfile (CHANGE_PW action) 에서 사용
    // ─────────────────────────────────────────────────────────

    // 비밀번호 일치 확인 (true = 일치)
    bool queryCheckPassword(int userId, const std::string& password);

    // 비밀번호 변경
    bool queryChangePassword(int userId, const std::string& newPassword);

    // ─────────────────────────────────────────────────────────
    //  [결제수단] Basehandler::handleGetProfile 에서 사용
    //  고객·사장 모두 카드를 등록/사용하므로 공통으로 관리
    // ─────────────────────────────────────────────────────────

    struct PaymentMethod {
        int         id         = 0;
        std::string alias;
        std::string maskedNum;
        std::string methodType;
        bool        isDefault  = false;
    };

    // 등록된 결제수단 목록 조회
    std::vector<PaymentMethod> queryPaymentMethods(int userId);

    // 결제수단 등록
    bool queryAddCard(int userId,
                      const std::string& alias,
                      const std::string& maskedNum,
                      const std::string& methodType);

    // 결제수단 삭제
    bool queryDeleteCard(int userId, int paymentMethodId);

    // 기본 결제수단 설정 (기존 기본 해제 → 신규 설정)
    bool querySetDefaultCard(int userId, int paymentMethodId);

    // ─────────────────────────────────────────────────────────
    //  [계정 상태] 로그아웃/탈퇴 등 공통 처리
    // ─────────────────────────────────────────────────────────

    // 계정 status 변경 ('ACTIVE' / 'SLEEP' / 'DELETED')
    bool querySetUserStatus(int userId, const std::string& status);

private:
    CommonDB() = default;
};
