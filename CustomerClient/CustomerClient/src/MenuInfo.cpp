#include "pch.h"
#include "MenuInfo.h"

MenuInfo::MenuInfo()
{
    Clear();
}

void MenuInfo::Clear()
{
    menuID = -1;
    menuName = "";
    price = 0;
    subCategory = "";
    menuImageUrl = "";
    optionGroups.clear();
}