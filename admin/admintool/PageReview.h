#pragma once

#include "PageBase.h"
#include <afxcmn.h>

/**
 * PageReview.h
 * ============================================================
 * 리뷰 관리 페이지이다.
 * 서버에서 리뷰 목록을 조회하고,
 * 리뷰 삭제 및 보이기/숨김 토글 기능을 제공한다.
 *
 * ★ 서버 연동:
 *   CMD_MANAGE_REVIEW (520) - 리뷰 관리 (list / delete / toggle_visibility)
 * ============================================================
 */
class PageReview : public PageBase
{
    DECLARE_DYNAMIC(PageReview)

public:
    PageReview(CWnd* pParent = nullptr);
    virtual ~PageReview();

    // PageBase 인터페이스
    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("리뷰 관리"); }

    // 리뷰 삭제 요청 (CMD_MANAGE_REVIEW, action=delete)
    void RequestDeleteReview(int reviewId);

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 메시지 핸들러
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnReviewDel();
    afx_msg void OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDrawReview(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    // 초기화 / 레이아웃
    void InitListCtrl();
    void RepositionList();

    // 서버에서 리뷰 목록 로드 (CMD_MANAGE_REVIEW = 520, action=list)
    void LoadDataFromServer();

    // 컨트롤
    CListCtrl   m_listReview;
    CImageList  m_imgList;
    CFont       m_font;
};