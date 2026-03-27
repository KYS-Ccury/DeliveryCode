#pragma once
// ================================================================
//  AddressManager.h  — 배달 주소 목록 관리 싱글턴
//
//  주소는 로컬 메모리에 관리하며 로그인 시 서버 기본 주소를 로드
//  최대 10개 주소 저장, 기본 주소(default) 1개 지정
// ================================================================
#include <string>
#include <vector>

struct AddressItem {
    std::string addr;       // 주소 문자열
    std::string label;      // 별칭 (예: "집", "회사")
    bool        isDefault = false;
};

class AddressManager
{
public:
    static AddressManager& GetInstance() {
        static AddressManager inst;
        return inst;
    }

    // 주소 목록 조작
    void AddAddress(const std::string& addr, const std::string& label = "");
    void DeleteAddress(int index);
    void SetDefault(int index);
    void Clear();

    // 조회
    const std::vector<AddressItem>& GetAll() const { return m_list; }
    std::string GetDefaultAddress() const;  // 기본 주소 반환 (없으면 "")
    int  GetDefaultIndex() const;
    int  Count() const { return (int)m_list.size(); }

    // 로그인 시 서버 주소를 첫 항목으로 초기화
    void InitFromLogin(const std::string& serverAddress);

private:
    AddressManager() {}
    std::vector<AddressItem> m_list;
};
