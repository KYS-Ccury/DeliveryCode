#include "pch.h"
#include "OrderManager.h"
#include "NetClient.h"
#include "Protocol.h" // CmdOwner, ClientType 등 포함
#include <string>

std::vector<OrderInfo> OrderManager::FetchOrdersFromServer(int ownerId) {
    std::vector<OrderInfo> orders;

    // 1. 서버에 보낼 요청(Request) JSON 구성
    json req;
    req["client_type"] = (int)ClientType::OWNER;
    req["owner_id"] = ownerId; // 로그인 시 저장해둔 사장님 고유 ID

    json res;
    // 2. 단발성 통신 수행 (REQ_ORDER_LIST: 304)
    if (CNetClient::SendRequest(CmdOwner::REQ_ORDER_LIST, req, res)) {

        // 3. 서버 응답 성공 확인
        if (res.contains("status") && res["status"] == Status::SUCCESS) {

            // 4. 주문 목록(orders 배열) 파싱
            if (res.contains("orders") && res["orders"].is_array()) {
                for (const auto& item : res["orders"]) {
                    OrderInfo info;

                    // JSON 데이터 추출 및 형변환
                    info.nID = item.value("order_id", 0);

                    // UTF-8로 넘어온 문자열을 MFC Unicode(CString)로 변환
                    std::string menuName = item.value("menu_name", "");
                    info.strMenu = CA2T(menuName.c_str(), CP_UTF8);

                    std::string status = item.value("status", "대기중");
                    info.strStatus = CA2T(status.c_str(), CP_UTF8);

                    // 가격은 int로 받아서 콤마(,) 포함 문자열로 변환 (예: 20000 -> 20,000)
                    int price = item.value("total_price", 0);
                    info.strPrice.Format(_T("%d"), price); // 필요 시 별도 콤마 포맷 함수 적용

                    orders.push_back(info);
                }
            }
        }
    }

    // 서버 통신 실패 시 빈 벡터 반환 (또는 에러 처리가 필요하면 여기서 로깅)
    return orders;
}


// OrderManager.cpp 하단에 추가
bool OrderManager::AcceptOrder(int orderId, int estimatedMinutes) {
    json req;
    req["client_type"] = (int)ClientType::OWNER;
    req["order_id"] = orderId;
    req["estimated_minutes"] = estimatedMinutes;

    json res;
    // CmdOwner::REQ_ACCEPT_ORDER (305) 호출
    if (CNetClient::SendRequest(CmdOwner::REQ_ACCEPT_ORDER, req, res)) {
        return res.contains("status") && res["status"] == Status::SUCCESS;
    }
    return false;
}

bool OrderManager::RejectOrder(int orderId, const CString& reason) {
    json req;
    req["client_type"] = (int)ClientType::OWNER;
    req["order_id"] = orderId;
    req["reason"] = std::string(CT2CA(reason)); // CString(UTF-16) -> UTF-8 String

    json res;
    // CmdOwner::REQ_REJECT_ORDER (306) 호출
    if (CNetClient::SendRequest(CmdOwner::REQ_REJECT_ORDER, req, res)) {
        return res.contains("status") && res["status"] == Status::SUCCESS;
    }
    return false;
}