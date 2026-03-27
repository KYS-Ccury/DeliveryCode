#pragma once
#include <afxwin.h>
#include <afxcmn.h>

class COrderDetailManager
{
public:
    // 상세 영역 전체 초기화 (리스트 헤더, 폰트 적용 등)
    static void InitArea(CWnd* pParent, CFont& font);

    // 상세 리스트에 데이터를 채우는 기능 (나중에 클릭 시 호출용)
    static void UpdateList(CWnd* pParent, CString strMenuName = _T(""));
    static void InitMainList(CWnd* pParent); // 메인 리스트 헤더 세팅

    static void AdjustCookTime(CWnd* pParent, int nDelta);

protected:
    afx_msg void OnBnClickedBtnTimePlus();  // [+] 버튼용
    afx_msg void OnBnClickedBtnTimeMinus(); // [-] 버튼용
};