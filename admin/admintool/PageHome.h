#pragma once

#include "PageBase.h"
#include <functional>

/**
 * PageHome.h
 * ============================================================
 * 대시보드(홈) 페이지이다.
 * 서버에서 주문/라이더/리뷰 카운트를 가져와 표시한다.
 *
 * ★ 서버 연동:
 *   CMD_ORDER_MONITOR (510) - 대기 주문 수
 *   CMD_RIDER_STATUS  (511) - 라이더 현황 수
 *   CMD_MANAGE_REVIEW (520) - 리뷰 수
 * ============================================================
 */
class PageHome : public PageBase
{
    DECLARE_DYNAMIC(PageHome)

public:
    PageHome(CWnd* pParent = nullptr);
    virtual ~PageHome();

    // PageBase 인터페이스
    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("홈"); }

    // 대시보드 카운트를 설정한다 (OnPaint에서 사용).
    void SetCounts(int nOrder, int nDispatch, int nReview);

    // MainDialog에서 설정하는 페이지 이동 콜백
    std::function<void()> m_fnGoInquiry;
    std::function<void()> m_fnGoReview;
    std::function<void()> m_fnGoDispatch;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 메시지 핸들러
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    // 서버에서 대시보드 데이터를 로드한다.
    void LoadDataFromServer();

    // 카드 하나를 그린다.
    void DrawCard(CDC* pDC, const CRect& rc, const CString& strTitle,
        int nCount, COLORREF clrAccent);

    // 대시보드 카운트
    int m_nOrderCount;
    int m_nDispatchCount;
    int m_nReviewCount;

    // 폰트
    CFont m_fontTitle;
    CFont m_fontCount;
};