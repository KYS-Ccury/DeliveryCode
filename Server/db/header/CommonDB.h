#pragma once
#include <string>
#include <nlohmann/json.hpp>

class CommonDB {
public:
    // MiddleHandler에서 호출할 1100번대 라우팅 전담 진입점
    static nlohmann::json process(uint16_t dbProtocol, const nlohmann::json& reqJson);

private:
    // 공통 회원가입 (users 테이블에 INSERT)
    static nlohmann::json signup(const nlohmann::json& reqJson);
    
    // 공통 로그인 (users 테이블에서 ID/PW 확인 후 user_id, client_type 반환)
    static nlohmann::json login(const nlohmann::json& reqJson);
};