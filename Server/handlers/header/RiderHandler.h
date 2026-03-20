#pragma once
#include <string>
#include <stdint.h>
class Session;

class RiderHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);
};