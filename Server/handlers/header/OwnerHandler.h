#pragma once
#include <string>
#include <cstdint>

class Session;

class OwnerHandler {
public:
    static void process(Session* session, uint16_t protocol, const std::string& jsonBody);

private:
    static void handleLogin       (Session* s, const std::string& body); // 101
    static void handleStoreInfo   (Session* s, const std::string& body); // 300
    static void handleUpdateStore (Session* s, const std::string& body); // 301
    static void handleAddMenu     (Session* s, const std::string& body); // 302
    static void handleSoldOut     (Session* s, const std::string& body); // 303
    static void handleOrderList   (Session* s, const std::string& body); // 304
    static void handleAcceptOrder (Session* s, const std::string& body); // 305
    static void handleRejectOrder (Session* s, const std::string& body); // 306
    static void handleCookingDone (Session* s, const std::string& body); // 307
    static void handleSalesStats  (Session* s, const std::string& body); // 308
    static void handleChangeStatus(Session* s, const std::string& body); // 309
};
