#pragma once
#include <afxwin.h>
#include <afxcmn.h>

class COrderDetailManager
{
public:
    static void InitArea(CWnd* pParent, CFont& font);
    static void InitMainList(CWnd* pParent);

    // 🚨 [수정] 데이터 갱신 시 표와 텍스트를 모두 바꾸도록 인자 추가
    static void UpdateList(CWnd* pParent, CString strMenuName = _T(""));

    static void AdjustCookTime(CWnd* pParent, int nDelta);

    // 🚨 [추가] 출력 버튼 기능
    static void ShowPrintReceipt(CWnd* pParent);
    static void ShowDeliveryGuide(CWnd* pParent);

protected:
    afx_msg void OnBnClickedBtnTimePlus();
    afx_msg void OnBnClickedBtnTimeMinus();
};