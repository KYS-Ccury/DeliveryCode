#pragma once

#ifndef __AFXWIN_H__
#error "pch.h를 먼저 포함해야 합니다."
#endif

#include "resource.h"

class CAdminToolApp : public CWinAppEx
{
public:
    CAdminToolApp();
    virtual BOOL InitInstance();
    DECLARE_MESSAGE_MAP()
};

class admintool
{
};
