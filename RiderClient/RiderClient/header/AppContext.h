#pragma once
#include "SocketManager.h"
#include "RiderSession.h"

class AppContext {
public:
    static AppContext& Get() {
        static AppContext inst;
        return inst;
    }
    SocketManager socket;
    RiderSession  session;
    CurrentOrder  currentOrder;
private:
    AppContext()  = default;
    ~AppContext() = default;
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;
};
