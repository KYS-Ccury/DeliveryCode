// ================================================================
//  AddressManager.cpp
//
//  [수정 내용]
//    1. AddAddress   → 서버에 REQ_SAVE_ADDRESS    (214) 전송
//    2. DeleteAddress → 서버에 REQ_DELETE_ADDRESS  (215) 전송
//    3. SetDefault   → 서버에 REQ_DEFAULT_ADDRESS  (216) 전송
//    4. RequestAddressesFromServer() → REQ_GET_ADDRESSES (213) 전송
//    5. OnAddressListResponse()     → 213 응답 파싱 → m_list 재구성
//    6. OnSaveAddressResponse()     → 214 응답에서 address_id 업데이트
//
//  서버 연동이 없는 환경(IsConnected()==false)에서도 메모리 동작은 유지됨.
// ================================================================
#include "pch.h"
#include "AddressManager.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

// ================================================================
//  내부 JSON 파싱 헬퍼 (수동 구현 — nlohmann 미포함 헤더 환경 대비)
// ================================================================
std::string AddressManager::ParseStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}

int AddressManager::ParseInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); }
    catch (...) { return -1; }
}

bool AddressManager::ParseBool(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos += token.size();
    return json.substr(pos, 4) == "true";
}

// ================================================================
//  SQL 이스케이프용 문자열 이스케이프 (JSON 문자열 내 따옴표 처리)
// ================================================================
static std::string EscapeJson(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

// ================================================================
//  AddAddress
//  메모리에 추가 + 서버에 REQ_SAVE_ADDRESS (214) 전송
//  서버 응답(OnSaveAddressResponse)에서 addressId 업데이트
// ================================================================
void AddressManager::AddAddress(const std::string& addr, const std::string& label)
{
    if (addr.empty()) return;

    // 중복 체크
    for (const auto& a : m_list)
        if (a.addr == addr) return;

    AddressItem item;
    item.addressId = 0;          // 서버 응답 전까지는 0
    item.addr      = addr;
    item.label     = label.empty() ? addr : label;
    item.isDefault = m_list.empty(); // 첫 번째이면 기본으로
    m_list.push_back(item);

    // ★ 서버에 저장 요청
    auto& net = NetworkManager::GetInstance();
    if (net.IsConnected()) {
        std::string token     = AuthManager::GetInstance().GetAccessToken();
        std::string safeAddr  = EscapeJson(addr);
        std::string safeLabel = EscapeJson(item.label);
        std::string isDefault = item.isDefault ? "true" : "false";

        std::string json =
            "{\"token\":\"" + token + "\","
            "\"address\":\"" + safeAddr + "\","
            "\"label\":\"" + safeLabel + "\","
            "\"is_default\":" + isDefault + "}";

        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCustomer::REQ_SAVE_ADDRESS, json);
    }
}

// ================================================================
//  DeleteAddress
//  메모리에서 제거 + 서버에 REQ_DELETE_ADDRESS (215) 전송
//  addressId == 0 이면 서버에 미등록된 항목 → 전송 생략
// ================================================================
void AddressManager::DeleteAddress(int index)
{
    if (index < 0 || index >= (int)m_list.size()) return;

    // ★ 서버에 삭제 요청 (addressId가 있는 경우만)
    auto& net = NetworkManager::GetInstance();
    if (net.IsConnected() && m_list[index].addressId > 0) {
        std::string token = AuthManager::GetInstance().GetAccessToken();
        std::string json =
            "{\"token\":\"" + token + "\","
            "\"address_id\":" + std::to_string(m_list[index].addressId) + "}";

        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCustomer::REQ_DELETE_ADDRESS, json);
    }

    bool wasDefault = m_list[index].isDefault;
    m_list.erase(m_list.begin() + index);

    // 삭제한 게 기본 주소였으면 첫 번째를 기본으로
    if (wasDefault && !m_list.empty())
        m_list[0].isDefault = true;
}

// ================================================================
//  SetDefault
//  메모리 갱신 + 서버에 REQ_DEFAULT_ADDRESS (216) 전송
// ================================================================
void AddressManager::SetDefault(int index)
{
    if (index < 0 || index >= (int)m_list.size()) return;

    for (auto& a : m_list) a.isDefault = false;
    m_list[index].isDefault = true;

    // ★ 서버에 기본 주소 변경 요청 (addressId가 있는 경우만)
    auto& net = NetworkManager::GetInstance();
    if (net.IsConnected() && m_list[index].addressId > 0) {
        std::string token = AuthManager::GetInstance().GetAccessToken();
        std::string json =
            "{\"token\":\"" + token + "\","
            "\"address_id\":" + std::to_string(m_list[index].addressId) + "}";

        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCustomer::REQ_DEFAULT_ADDRESS, json);
    }
}

// ================================================================
//  GetDefaultAddress / GetDefaultIndex / Clear
// ================================================================
std::string AddressManager::GetDefaultAddress() const
{
    for (const auto& a : m_list)
        if (a.isDefault) return a.addr;
    if (!m_list.empty()) return m_list[0].addr;
    return "";
}

int AddressManager::GetDefaultIndex() const
{
    for (int i = 0; i < (int)m_list.size(); i++)
        if (m_list[i].isDefault) return i;
    return m_list.empty() ? -1 : 0;
}

void AddressManager::Clear()
{
    m_list.clear();
}

// ================================================================
//  RequestAddressesFromServer
//  ★ 로그인 성공 직후 호출 → 서버에 REQ_GET_ADDRESSES (213) 전송
//  콜백은 MainHomeDlg 또는 AddressDlg에서 등록 후 OnAddressListResponse 호출
// ================================================================
void AddressManager::RequestAddressesFromServer()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json  = "{\"token\":\"" + token + "\"}";

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_GET_ADDRESSES, json);
}

// ================================================================
//  OnAddressListResponse
//  ★ REQ_GET_ADDRESSES (213) 서버 응답 수신 후 호출
//
//  서버 응답 형식:
//  {
//    "status": 2000,
//    "addresses": [
//      {"address_id":1, "address":"광주시 북구 용봉동", "label":"집", "is_default":true},
//      {"address_id":2, "address":"광주시 서구 치평동", "label":"회사", "is_default":false}
//    ]
//  }
// ================================================================
void AddressManager::OnAddressListResponse(const std::string& jsonBody)
{
    m_list.clear();

    // "addresses" 배열 추출
    const std::string arrayKey = "\"addresses\":[";
    auto startPos = jsonBody.find(arrayKey);
    if (startPos == std::string::npos) return;

    startPos += arrayKey.size();
    auto endPos = jsonBody.rfind(']');
    if (endPos == std::string::npos || endPos <= startPos) return;

    std::string arrayStr = jsonBody.substr(startPos, endPos - startPos);

    // 각 객체 {} 파싱
    size_t cursor = 0;
    while (cursor < arrayStr.size()) {
        auto objStart = arrayStr.find('{', cursor);
        if (objStart == std::string::npos) break;
        auto objEnd = arrayStr.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string obj = arrayStr.substr(objStart, objEnd - objStart + 1);

        AddressItem item;
        item.addressId = ParseInt(obj, "address_id");
        item.addr      = ParseStr(obj, "address");
        item.label     = ParseStr(obj, "label");
        item.isDefault = ParseBool(obj, "is_default");

        if (!item.addr.empty())
            m_list.push_back(item);

        cursor = objEnd + 1;
    }
}

// ================================================================
//  OnSaveAddressResponse
//  ★ REQ_SAVE_ADDRESS (214) 응답에서 새로 발급된 address_id를
//    m_list의 마지막 항목(방금 추가한 것)에 저장
//
//  서버 응답 형식:
//  { "status": 2000, "address_id": 5 }
// ================================================================
void AddressManager::OnSaveAddressResponse(const std::string& jsonBody)
{
    int newId = ParseInt(jsonBody, "address_id");
    if (newId <= 0) return;

    // addressId == 0 인 항목(서버 미등록)을 찾아 ID 부여
    for (auto& item : m_list) {
        if (item.addressId == 0) {
            item.addressId = newId;
            break;
        }
    }
}

// ================================================================
//  InitFromLogin  [구버전 호환 — 더 이상 사용 안 함]
//  LoginDlg에서 RequestAddressesFromServer()로 대체됨
//  단, 서버 연결 없이 오프라인 테스트 시에는 여전히 유효
// ================================================================
void AddressManager::InitFromLogin(const std::string& serverAddress)
{
    m_list.clear();
    if (!serverAddress.empty()) {
        AddressItem item;
        item.addressId = 0;
        item.addr      = serverAddress;
        item.label     = serverAddress;
        item.isDefault = true;
        m_list.push_back(item);
    }
}
