// CommonHandler.h
#pragma once
#include "Session.h"
#include <nlohmann/json.hpp>

class CommonHandler {
public:
    static void handleSignup(Session* session, const nlohmann::json& reqBody);
    static void handleLogin(Session* session, const nlohmann::json& reqBody);
};