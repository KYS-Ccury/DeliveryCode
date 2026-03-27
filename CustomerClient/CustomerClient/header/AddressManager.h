#pragma once
// ================================================================
//  AddressManager.h  — 배달 주소 목록 관리 싱글턴
//
//  [수정] 주소를 서버 DB에 영구 저장
//    - AddressItem에 addressId(DB PK) 필드 추가
//    - AddAddress / DeleteAddress / SetDefault 시 서버 패킷 전송
//    - 로그인 후 RequestAddressesFromServer() 호출로 DB에서 목록 복원
//    - OnAddressListResponse()로 서버 응답 파싱
// ================================================================
#include <string>
#include <vector>

// AddressItem: DB의 address_id 포함
struct AddressItem {
    int         addressId = 0;   // ★ DB의 user_addresses.address_id (서버 미등록 시 0)
    std::string addr;            // 주소 문자열
    std::string label;           // 별칭 (예: "집", "회사")
    bool        isDefault = false;
};

class AddressManager
{
public:
    static AddressManager& GetInstance() {
        static AddressManager inst;
        return inst;
    }

    // ── 주소 목록 조작 ────────────────────────────────────────
    // ★ 주소 추가: 메모리 + 서버 DB 저장
    void AddAddress(const std::string& addr, const std::string& label = "");

    // ★ 주소 삭제: 메모리 + 서버 DB 삭제
    void DeleteAddress(int index);

    // ★ 기본 주소 설정: 메모리 + 서버 DB 갱신
    void SetDefault(int index);

    // 전체 초기화 (로그아웃 시 호출)
    void Clear();

    // ── 조회 ──────────────────────────────────────────────────
    const std::vector<AddressItem>& GetAll() const { return m_list; }
    std::string GetDefaultAddress() const;
    int  GetDefaultIndex() const;
    int  Count() const { return (int)m_list.size(); }

    // ── 서버 연동 ─────────────────────────────────────────────
    // ★ 로그인 성공 직후 호출: 서버에서 주소 목록 요청 (213)
    void RequestAddressesFromServer();

    // ★ 서버 응답(213) 수신 후 호출: JSON 파싱 → m_list 재구성
    void OnAddressListResponse(const std::string& jsonBody);

    // ★ 서버 저장 응답(214) 수신 후 호출: address_id 업데이트
    void OnSaveAddressResponse(const std::string& jsonBody);

    // [구버전 호환] 로그인 시 서버 단일 주소로 초기화 — 더 이상 사용 안 함
    // RequestAddressesFromServer()로 대체됨
    void InitFromLogin(const std::string& serverAddress);

private:
    AddressManager() {}
    std::vector<AddressItem> m_list;

    // JSON 수동 파싱 헬퍼 (nlohmann json 미포함 환경 대비)
    static std::string ParseStr(const std::string& json, const std::string& key);
    static int         ParseInt(const std::string& json, const std::string& key);
    static bool        ParseBool(const std::string& json, const std::string& key);
};
