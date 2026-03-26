#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소
#include <afxext.h>         // MFC 확장
#include <afxdisp.h>        // MFC 자동화 클래스

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>       // Internet Explorer 4 공용 컨트롤에 대한 MFC 지원
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // Windows 공용 컨트롤에 대한 MFC 지원
#endif

#include <afxcontrolbars.h> // 리본 및 컨트롤 막대 지원