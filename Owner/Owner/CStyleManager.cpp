#include "pch.h"
#include "CStyleManager.h"

void CStyleManager::ApplyFont(CWnd* pWnd, CFont& font, int nSize, bool bBold, CString strFontName)
{
	// 방어 코드: 컨트롤이 없으면 바로 종료
	if (pWnd == nullptr) return;

	// 기존에 폰트가 생성되어 있다면 지우고 새로 만듭니다. (메모리 누수 방지)
	if (font.GetSafeHandle() != NULL) {
		font.DeleteObject();
	}

	// 굵게 할지 말지 결정
	int nWeight = bBold ? FW_BOLD : FW_NORMAL;

	// 폰트 생성 로직 (여기에 복잡한 코드를 다 숨깁니다!)
	font.CreateFont(
		nSize, 0, 0, 0, nWeight,
		FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, strFontName);

	// 대상 컨트롤에 폰트 적용
	pWnd->SetFont(&font);
}