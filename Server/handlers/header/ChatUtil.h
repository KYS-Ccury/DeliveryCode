// ============================================================
//  ChatUtil.h [채팅 공통 유틸리티]
// ============================================================
#pragma once
#include "Session.h"
#include <string>
#include <nlohmann/json.hpp>

class ChatUtil {
public:
    static std::string escStr(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '\'' || c == '\\' || c == '"') out += '\\';
            out += c;
        }
        return out;
    }

    static void sendErr(Session* s, uint16_t proto, uint16_t code, const std::string& msg, ClientType ct) {
        nlohmann::json r; 
        r["status"] = code; 
        r["message"] = msg;
        s->sendPacket(static_cast<uint8_t>(ct), proto, r.dump());
    }
};