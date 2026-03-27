// MiddleHandler.h
#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>

class MiddleHandler {
public:
    // 네트워크 계층에서 넘어온 모든 DB 요청을 중앙에서 라우팅
    static nlohmann::json processDBRequest(uint16_t dbProtocol, const nlohmann::json& reqJson);
};