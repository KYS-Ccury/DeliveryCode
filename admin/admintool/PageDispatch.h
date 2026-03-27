/**
 * PageDispatch.h
 * ============================================================
 * ★ 수정: RequestStatusChange() 추가
 * ============================================================
 */

#pragma once

#include "PageBase.h"
#include <afxcmn.h>

class PageDispatch : public PageBase
{
    DECLARE_DYNAMIC(PageDispatch)

public:
    PageDispatch(CWnd* pParent = nullptr);
    virtual ~PageDispatch();

    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("배차 관리"); }

    void RequestForceDispatch(int orderId, int riderId);
    void RequestForceCancel(int orderId);
    void RequestStatusChange(int nItemIndex, const CString& strNewStatus);

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnDispatchDel();
    afx_msg void OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    void InitListCtrl();
    void RepositionList();
    void LoadDataFromServer();

    CListCtrl   m_listDispatch;
    CImageList  m_imgList;
    CFont       m_font;
};