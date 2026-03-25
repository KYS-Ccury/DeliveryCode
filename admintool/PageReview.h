#pragma once
#include "PageBase.h"
#include "resource.h"

class PageReview : public PageBase
{
    DECLARE_DYNAMIC(PageReview)

public:
    PageReview(CWnd* pParent = nullptr);
    virtual ~PageReview();

    enum { IDD = IDD_PAGE_REVIEW };

    virtual CString GetPageName() override { return _T("Review"); }
    virtual void LoadData() override;
    virtual void SaveData() override;

protected:
    CListCtrl  m_listReview;
    CFont      m_font;
    CImageList m_imgList;

    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    void InitListCtrl();
    void RepositionList();
    void InsertDummyData();

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnReviewDel();
    afx_msg void OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDrawReview(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
};