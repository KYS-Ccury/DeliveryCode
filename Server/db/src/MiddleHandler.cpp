// MiddleHandler.cpp
#include "MiddleHandler.h"
#include "Protocol.h"
#include "CommonDB.h"
#include "RiderDBHandler.h"
#include "CustomerDBHandler.h"
#include "OwnerDB.h"
#include "AdminDBHandler.h"
#include <iostream>

using json = nlohmann::json;

json MiddleHandler::processDBRequest(uint16_t dbProtocol, const json& reqJson) {
    // 1000번대 단위로 대분류를 나누어 각 DB 핸들러로 라우팅합니다.
    
    if (dbProtocol >= 1100 && dbProtocol < 1200) {
        // [1100번대] 공통 / 인증 DB 처리
        return CommonDB::process(dbProtocol, reqJson);
    } 
    // else if (dbProtocol >= 1200 && dbProtocol < 1300) {
    //     // [1200번대] 고객 DB 처리
    //     // return CustomerDBHandler::process(dbProtocol, reqJson);
    // } 
    else if (dbProtocol >= 1300 && dbProtocol < 1400) {
        // [1300번대] 사장님 DB 처리
        return OwnerDB::process(dbProtocol, reqJson);
    } 
    else if (dbProtocol >= 1400 && dbProtocol < 1500) {
        //// [1400번대] 라이더 DB 처리
        return RiderDBHandler::process(dbProtocol, reqJson);
    } 
    // else if (dbProtocol >= 1500 && dbProtocol < 1600) {
        //// [1500번대] 관리자 DB 처리
        // return AdminDBHandler::process(dbProtocol, reqJson);
    // }

    // 매칭되는 프로토콜이 없을 경우
    std::cerr << "[MiddleHandler] Unknown DB Protocol: " << dbProtocol << std::endl;
    json errRes;
    errRes["status"] = Status::SERVER_ERROR;
    errRes["message"] = "Invalid Internal DB Protocol";
    return errRes;
}