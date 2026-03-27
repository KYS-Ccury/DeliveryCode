#pragma execution_character_set("utf-8")
#include "pch.h"
#include "COrderDetailManager.h"
#include "CStyleManager.h" // 폰트 적용을 위해 기존 매니저 포함
#include "resource.h"      // ID 사용을 위해 포함

void COrderDetailManager::InitArea(CWnd* pParent, CFont& font)
{
    if (!pParent) return;

    // 폰트 적용
    CStyleManager::ApplyFont(pParent->GetDlgItem(IDC_STATIC_DETAIL_TITLE), font, 18, true, _T("맑은 고딕"));

    CListCtrl* pDetailList = (CListCtrl*)pParent->GetDlgItem(IDC_LIST_ORDER_MENU);
    if (pDetailList) // 👈 안전하게 if 체크 추가!
    {
        pDetailList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        while (pDetailList->DeleteColumn(0));

        pDetailList->InsertColumn(0, _T("메뉴"), LVCFMT_LEFT, 230);
        pDetailList->InsertColumn(1, _T("수량"), LVCFMT_CENTER, 45);
        pDetailList->InsertColumn(2, _T("금액"), LVCFMT_RIGHT, 90);
    }

    // 이제 헤더에서 기본값을 설정했으므로, 아래 호출이 정상 작동합니다!
    UpdateList(pParent);
}

void COrderDetailManager::UpdateList(CWnd* pParent, CString strMenuName)
{
    CListCtrl* pDetailList = (CListCtrl*)pParent->GetDlgItem(IDC_LIST_ORDER_MENU);
    if (!pDetailList) return;

    pDetailList->DeleteAllItems();

    // 1. 메뉴명에 '치킨'이 포함된 경우
    if (!strMenuName.IsEmpty() && strMenuName.Find(_T("치킨")) != -1)
    {
        int nIdx = pDetailList->InsertItem(0, _T("황금 올리브 (기본)"));
        pDetailList->SetItemText(nIdx, 1, _T("1"));
        pDetailList->SetItemText(nIdx, 2, _T("20,000원"));

        nIdx = pDetailList->InsertItem(1, _T("치킨무 추가"));
        pDetailList->SetItemText(nIdx, 1, _T("1"));
        pDetailList->SetItemText(nIdx, 2, _T("500원"));
    }
    // 2. 초기 상태이거나 다른 메뉴인 경우
    else
    {
        int nIdx = pDetailList->InsertItem(0, _T("선택된 주문이 없습니다"));
        pDetailList->SetItemText(nIdx, 1, _T("-"));
        pDetailList->SetItemText(nIdx, 2, _T("0원"));
    }
}

void COrderDetailManager::InitMainList(CWnd* pParent) {
    CListCtrl* pList = (CListCtrl*)pParent->GetDlgItem(IDC_LIST_ORDER);
    if (pList) {
        // [핵심] 기존에 컬럼이 있다면 모두 삭제 (중복 생성 방지)
        while (pList->DeleteColumn(0));

        pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        pList->InsertColumn(0, _T("No"), LVCFMT_LEFT, 40);
        pList->InsertColumn(1, _T("메뉴명"), LVCFMT_LEFT, 140);
        pList->InsertColumn(2, _T("금액"), LVCFMT_RIGHT, 80);
        pList->InsertColumn(3, _T("상태"), LVCFMT_CENTER, 80);
    }
}

// 시간 조절 로직 구현
void COrderDetailManager::AdjustCookTime(CWnd* pParent, int nDelta) {
    if (!pParent) return;

    CString strTime;
    pParent->GetDlgItemText(IDC_STATIC_COOK_TIME, strTime);

    int nTime = _ttoi(strTime); // "15분"에서 숫자 15만 추출
    nTime += nDelta;

    if (nTime < 5) nTime = 5;   // 최소 5분
    if (nTime > 60) nTime = 60; // 최대 60분

    strTime.Format(_T("%d분"), nTime);
    pParent->SetDlgItemText(IDC_STATIC_COOK_TIME, strTime);
}