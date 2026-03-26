#pragma once
#include "MariaDBManager.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class CommonDB {
public:
    static CommonDB& getInstance() {
        static CommonDB inst;
        return inst;
    }

    int queryLogin(const std::string& loginId,
                   const std::string& password,
                   const std::string& role);


    bool queryCheckDuplicateId(const std::string& loginId);

    // users 테이블 INSERT
    // 반환: 새로 생성된 user_id (실패 시 -1)
    int queryInsertUser(const std::string& loginId,
                        const std::string& password,
                        const std::string& role,
                        const std::string& name,
                        const std::string& phone,
                        const std::string& address);

    struct UserBasic {
        std::string name;
        std::string phone;
        std::string address;
        bool        found = false;
    };

    // user_id → name, phone, address
    UserBasic queryUserBasic(int userId);

    // 비밀번호 일치 확인 (true = 일치)
    bool queryCheckPassword(int userId, const std::string& password);

    // 비밀번호 변경
    bool queryChangePassword(int userId, const std::string& newPassword);

    struct PaymentMethod {
        int         id         = 0;
        std::string alias;
        std::string maskedNum;
        std::string methodType;
        bool        isDefault  = false;
    };


    std::vector<PaymentMethod> queryPaymentMethods(int userId);


    bool queryAddCard(int userId,
                      const std::string& alias,
                      const std::string& maskedNum,
                      const std::string& methodType);


    bool queryDeleteCard(int userId, int paymentMethodId);

    // 기본 결제수단 설정 (기존 기본 해제 → 신규 설정)
    bool querySetDefaultCard(int userId, int paymentMethodId);

    // 계정 status 변경 ('ACTIVE' / 'SLEEP' / 'DELETED')
    bool querySetUserStatus(int userId, const std::string& status);

    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);
    static nlohmann::json signup(const nlohmann::json& reqBody);
    static nlohmann::json login(const nlohmann::json& reqBody);

private:
    CommonDB() = default;
};
