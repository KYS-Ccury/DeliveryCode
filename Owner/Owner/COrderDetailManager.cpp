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
    if (!pParent) return;

    CListCtrl* pDetailList = (CListCtrl*)pParent->GetDlgItem(IDC_LIST_ORDER_MENU);
    if (!pDetailList) return;

    pDetailList->DeleteAllItems();

    // ========================================================
    // 1. 선택된 메뉴에 따라 "표(List)" 데이터 변경
    // ========================================================
    if (strMenuName == _T("후라이드 치킨")) {
        int nIdx = pDetailList->InsertItem(0, _T("후라이드 치킨"));
        pDetailList->SetItemText(nIdx, 1, _T("1"));
        pDetailList->SetItemText(nIdx, 2, _T("20,000원"));

        nIdx = pDetailList->InsertItem(1, _T("치킨무 추가"));
        pDetailList->SetItemText(nIdx, 1, _T("1"));
        pDetailList->SetItemText(nIdx, 2, _T("500원"));
        
        // 🚨 2. [추가] 우측 하단 "주문 정보 및 요청사항"도 같이 변경!!
        pParent->SetDlgItemText(IDC_STATIC_REQ_SHOP, _T("리뷰 이벤트 참여합니다! 치즈볼 주세요."));
        pParent->SetDlgItemText(IDC_STATIC_REQ_DELIVERY, _T("문 앞에 두고 벨 눌러주세요."));
    }
    else {
        int nIdx = pDetailList->InsertItem(0, _T("선택된 주문이 없습니다"));
        pDetailList->SetItemText(nIdx, 1, _T("-"));
        pDetailList->SetItemText(nIdx, 2, _T("0원"));

        // 🚨 2. [추가] 빈 주문일 때 텍스트 초기화
        pParent->SetDlgItemText(IDC_STATIC_REQ_SHOP, _T("없음"));
        pParent->SetDlgItemText(IDC_STATIC_REQ_DELIVERY, _T("없음"));
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

void COrderDetailManager::ShowPrintReceipt(CWnd* pParent)
{
    if (!pParent) return;

    CString strSummary, strReqShop;
    pParent->GetDlgItemText(IDC_STATIC_DETAIL_SUMMARY, strSummary);
    pParent->GetDlgItemText(IDC_STATIC_REQ_SHOP, strReqShop);

    // 실제 영수증처럼 보이도록 문자열 예쁘게 꾸미기
    CString strReceipt;
    strReceipt.Format(
        _T("=============== [ 주문 전표 ] ===============\n\n")
        _T(" [주문 내역]\n  %s\n\n")
        _T("---------------------------------------------\n")
        _T(" [가게 요청사항]\n  %s\n\n")
        _T("=============================================\n")
        _T("\n※ 전표 출력이 완료되었습니다."),
        strSummary, strReqShop);

    pParent->MessageBox(strReceipt, _T("전표 출력기"), MB_OK | MB_ICONINFORMATION);
}

void COrderDetailManager::ShowDeliveryGuide(CWnd* pParent)
{
    if (!pParent) return;

    CString strReqDeli;
    pParent->GetDlgItemText(IDC_STATIC_REQ_DELIVERY, strReqDeli);

    CString strGuide;
    strGuide.Format(
        _T("=============== [ 배달 안내서 ] ===============\n\n")
        _T(" 📍 배달 목적지: 고객님 주소\n")
        _T(" 📞 고객 연락처: 050-XXXX-XXXX (안심번호)\n\n")
        _T("-----------------------------------------------\n")
        _T(" [라이더님 요청사항]\n  %s\n\n")
        _T("===============================================\n")
        _T("\n※ 안전 운전 부탁드립니다."),
        strReqDeli);

    pParent->MessageBox(strGuide, _T("배달 안내 출력기"), MB_OK | MB_ICONINFORMATION);
}