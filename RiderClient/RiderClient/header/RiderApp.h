#pragma once
#include <afxwin.h>

class RiderApp : public CWinApp {
public:
    RiderApp();
    virtual BOOL InitInstance() override;
    virtual int  ExitInstance() override;
    DECLARE_MESSAGE_MAP()
};

extern RiderApp theApp;
