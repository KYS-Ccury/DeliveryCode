#pragma once
#include <string>
#include <stdint.h>
class Session;

class OwnerHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);
};