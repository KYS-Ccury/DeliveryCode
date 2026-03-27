#pragma once
#include <afxwin.h>

class CStyleManager
{
public:
	// 컨트롤, 폰트 변수, 크기, 굵기 여부, 폰트 이름을 넘겨주면 알아서 세팅해 주는 만능 함수
	static void ApplyFont(CWnd* pWnd, CFont& font, int nSize, bool bBold, CString strFontName);
};