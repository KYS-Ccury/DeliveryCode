/**
 * PageHome.h
 * ============================================================
 * ★ 수정: OnLButtonDown, 미니리스트, 카드 CRect 멤버 추가
 * ============================================================
 */

#pragma once

#include "PageBase.h"
#include <functional>
#include <vector>

class PageHome : public PageBase
{
    DECLARE_DYNAMIC(PageHome)

public:
    PageHome(CWnd* pParent = nullptr);
    virtual ~PageHome();

    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("홈"); }

    void SetCounts(int nOrder, int nDispatch, int nReview);

    std::function<void()> m_fnGoInquiry;
    std::function<void()> m_fnGoReview;
    std::function<void()> m_fnGoDispatch;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    void LoadDataFromServer();
    void DrawCard(CDC* pDC, const CRect& rc, const CString& strTitle,
        int nCount, COLORREF clrAccent);
    void DrawMiniList(CDC* pDC, const CRect& rc, const CString& strTitle,
        const std::vector<CString>& items);

    int m_nOrderCount;
    int m_nDispatchCount;
    int m_nReviewCount;

    CRect m_rcCardOrder;
    CRect m_rcCardDispatch;
    CRect m_rcCardReview;
    CRect m_rcMiniOrders;
    CRect m_rcMiniReviews;

    std::vector<CString> m_recentOrders;
    std::vector<CString> m_recentReviews;

    CFont m_fontTitle;
    CFont m_fontCount;
    CFont m_fontMini;
    CFont m_fontMiniTitle;
};