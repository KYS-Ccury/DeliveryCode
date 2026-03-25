#pragma once
#include "PageBase.h"
#include "resource.h"

class PageDispatch : public PageBase
{
    DECLARE_DYNAMIC(PageDispatch)

public:
    PageDispatch(CWnd* pParent = nullptr);
    virtual ~PageDispatch();

    enum { IDD = IDD_PAGE_DISPATCH };

    virtual CString GetPageName() override { return _T("Dispatch"); }
    virtual void LoadData() override;
    virtual void SaveData() override;

protected:
    CListCtrl  m_listDispatch;
    CFont      m_font;
    CImageList m_imgList;

    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    void InitListCtrl();
    void RepositionList();
    void InsertDummyData();

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnDispatchDel();
    afx_msg void OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
};