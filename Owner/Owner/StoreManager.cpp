#include "pch.h"
#include "StoreManager.h"
#include "NetClient.h"
#include "Protocol.h"

bool StoreManager::UpdateStoreStatus(int ownerId, bool bOpen) {
    json req, res;
    req["client_type"] = (int)ClientType::OWNER;
    req["owner_id"] = ownerId;
    req["is_open"] = bOpen; // true(영업 시작) or false(영업 중지)

    // REQ_UPDATE_STATUS (316) 통신
    if (CNetClient::SendRequest(CmdOwner::REQ_UPDATE_STATUS, req, res)) {
        return res["status"] == Status::SUCCESS;
    }
    return false;
}