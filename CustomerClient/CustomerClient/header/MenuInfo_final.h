#pragma once
#include <string>
#include <vector>

struct MenuOption {
    int         optionItemID  = 0;    // DB: option_items.option_item_id
    int         optionGroupID = 0;    // DB: option_groups.option_group_id
    std::string optionName;           // DB: option_items.item_name
    int         optionPrice   = 0;    // DB: option_items.extra_price
    bool        isRequired    = false;// DB: option_groups.is_required
};

struct MenuInfo {
    int         menuID      = 0;    // DB: menus.menu_id
    std::string menuName;           // DB: menus.menu_name
    std::string description;        // DB: menus.description
    int         basePrice   = 0;    // DB: menus.price
    std::string subCategory;        // DB: menu_categories.category_name
    bool        isSoldOut   = false;// DB: menus.is_sold_out
    std::vector<MenuOption> options;
};
