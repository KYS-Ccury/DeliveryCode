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

    // SQL 이스케이프 유틸 — RiderDB 등 다른 DB 클래스에서도 사용
    static std::string escape(const std::string& s);

    int queryLogin(const std::string& loginId,
                   const std::string& password,
                   const std::string& role);

    bool queryCheckDuplicateId(const std::string& loginId);

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

    UserBasic queryUserBasic(int userId);

    bool queryCheckPassword(int userId, const std::string& password);

    bool queryChangePassword(int userId, const std::string& newPassword);

    struct PaymentMethod {
        int         id        = 0;
        std::string alias;
        std::string maskedNum;
        std::string methodType;
        bool        isDefault = false;
    };

    std::vector<PaymentMethod> queryPaymentMethods(int userId);

    bool queryAddCard(int userId,
                      const std::string& alias,
                      const std::string& maskedNum,
                      const std::string& methodType);

    bool queryDeleteCard(int userId, int paymentMethodId);

    bool querySetDefaultCard(int userId, int paymentMethodId);

    bool querySetUserStatus(int userId, const std::string& status);

    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);
    static nlohmann::json signup(const nlohmann::json& reqBody);
    static nlohmann::json login(const nlohmann::json& reqBody);

private:
    CommonDB() = default;
};
