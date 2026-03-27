// ================================================================
//  AddressManager.cpp
// ================================================================
#include "pch.h"
#include "AddressManager.h"

void AddressManager::AddAddress(const std::string& addr,
                                 const std::string& label)
{
    if (addr.empty()) return;
    // 중복 체크
    for (const auto& a : m_list)
        if (a.addr == addr) return;

    AddressItem item;
    item.addr      = addr;
    item.label     = label.empty() ? addr : label;
    item.isDefault = m_list.empty(); // 첫 번째면 기본으로
    m_list.push_back(item);
}

void AddressManager::DeleteAddress(int index)
{
    if (index < 0 || index >= (int)m_list.size()) return;
    bool wasDefault = m_list[index].isDefault;
    m_list.erase(m_list.begin() + index);
    // 삭제한 게 기본 주소였으면 첫 번째를 기본으로
    if (wasDefault && !m_list.empty())
        m_list[0].isDefault = true;
}

void AddressManager::SetDefault(int index)
{
    if (index < 0 || index >= (int)m_list.size()) return;
    for (auto& a : m_list) a.isDefault = false;
    m_list[index].isDefault = true;
}

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

void AddressManager::InitFromLogin(const std::string& serverAddress)
{
    m_list.clear();
    if (!serverAddress.empty()) {
        AddressItem item;
        item.addr      = serverAddress;
        item.label     = serverAddress;
        item.isDefault = true;
        m_list.push_back(item);
    }
}
