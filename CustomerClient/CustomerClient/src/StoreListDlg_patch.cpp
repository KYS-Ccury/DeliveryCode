#include "pch.h"

// StoreListDlg.cpp 갱신 – 서버 응답의 sub_categories 목록으로 탭 동적 구성
// (기존 하드코딩 {"전체","인기메뉴",...} 제거)

// OnInitDialog 내 서브카테고리 부분만 교체:
// ── 기존 코드 ──
//   m_vecSubCategories = { _T("전체"), _T("인기메뉴"), _T("세트메뉴"), _T("단품"), _T("음료") };

// ── 교체 코드 ──
//   UpdateMenuListUI(_T("전체"));  // 첫 로드 시 서버 응답에서 sub_categories 수신
//   → OnInitDialog에서 UpdateMenuListUI("전체") 호출 시
//     서버가 menus + sub_categories 배열을 함께 반환하므로
//     아래 UpdateMenuListUI 내에서 탭을 갱신

// ─────────────────────────────────────────────────
// UpdateMenuListUI 수정본 (StoreListDlg.cpp 내 해당 함수 교체)
// ─────────────────────────────────────────────────

/*
void StoreListDlg::UpdateMenuListUI(CString subCategory)
{
    m_listMenu.DeleteAllItems();
    m_vecMenuCache.clear();

    int storeID = OrderManager::GetInstance().GetCurrentStoreID();
    std::string sub = CT2A(subCategory);

    // REQ_MENU_LIST (201) 서버 요청
    // OrderManager::LoadMenuData 가 내부적으로 sub_categories도 반환
    auto [menus, subCats] = OrderManager::GetInstance().LoadMenuDataWithCats(storeID, sub);

    // ── 서브카테고리 탭 갱신 (첫 로드 시에만) ──
    if (!subCats.empty() && m_vecSubCategories.size() <= 1) {
        m_vecSubCategories.clear();
        for (const auto& c : subCats)
            m_vecSubCategories.push_back(CA2T(c.c_str()));
        if (m_wndScrollMenu.GetSafeHwnd())
            m_wndScrollMenu.SetMenuItems(m_vecSubCategories);
    }

    if (menus.empty()) {
        m_listMenu.InsertItem(0, _T("메뉴 정보를 불러올 수 없습니다."));
        return;
    }

    for (int i = 0; i < (int)menus.size(); ++i) {
        CString strName  = CA2T(menus[i].menuName.c_str());
        CString strDesc  = CA2T(menus[i].description.c_str());
        CString strPrice;
        strPrice.Format(_T("%d원"), menus[i].basePrice);

        int nRow = m_listMenu.InsertItem(i, strName);
        m_listMenu.SetItemText(nRow, 1, strDesc);
        m_listMenu.SetItemText(nRow, 2, strPrice);
        m_vecMenuCache.push_back(menus[i]);
    }
}
*/

// ─────────────────────────────────────────────────
// OrderManager에 추가할 메서드 (LoadMenuDataWithCats)
// ─────────────────────────────────────────────────
/*
// OrderManager.h에 추가:
std::pair<std::vector<MenuInfo>, std::vector<std::string>>
    LoadMenuDataWithCats(int storeID, const std::string& subCategory = "전체");

// OrderManager.cpp에 추가:
std::pair<std::vector<MenuInfo>, std::vector<std::string>>
OrderManager::LoadMenuDataWithCats(int storeID, const std::string& subCategory)
{
    std::vector<MenuInfo>   menus;
    std::vector<std::string> subCats;
    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_MENU_LIST,
        [&](uint16_t, const std::string& body) {
            try {
                auto res = json::parse(body);
                if (res["status"].get<int>() == Status::SUCCESS) {
                    // 메뉴 파싱 (기존 LoadMenuData 로직과 동일)
                    for (auto& m : res["menus"]) {
                        MenuInfo mi;
                        mi.menuID      = m.value("id",    0);
                        mi.menuName    = m.value("name",  "");
                        mi.description = m.value("desc",  "");
                        mi.basePrice   = m.value("price", 0);
                        mi.subCategory = m.value("sub_category", "");
                        mi.isSoldOut   = m.value("is_sold_out", false);
                        // option_groups 파싱
                        if (m.contains("option_groups")) {
                            for (auto& og : m["option_groups"]) {
                                for (auto& oi : og["options"]) {
                                    MenuOption opt;
                                    opt.optionName  = oi.value("name",  "");
                                    opt.optionPrice = oi.value("price", 0);
                                    opt.optionItemID= oi.value("option_id", 0);
                                    mi.options.push_back(opt);
                                }
                            }
                        }
                        menus.push_back(mi);
                    }
                    // 서브카테고리 목록
                    if (res.contains("sub_categories"))
                        for (auto& c : res["sub_categories"])
                            subCats.push_back(c.get<std::string>());
                }
            } catch (...) {}
            SetEvent(hEvent);
        }
    );

    json req;
    req["token"]        = AuthManager::GetInstance().GetAccessToken();
    req["store_id"]     = storeID;
    req["sub_category"] = subCategory;
    NetworkManager::GetInstance().SendPacket(
        static_cast<uint8_t>(ClientType::CUSTOMER),
        CmdCustomer::REQ_MENU_LIST,
        req.dump()
    );

    WaitForSingleObject(hEvent, 5000);
    CloseHandle(hEvent);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MENU_LIST);
    return {menus, subCats};
}
*/

// 이 파일은 수정 패치 가이드입니다.
// 위 주석 내용을 OrderManager.h/.cpp 와 StoreListDlg.cpp에 적용하세요.
