#pragma once

#include "PageBase.h"
#include <afxcmn.h>

/**
 * PageDispatch.h
 * ============================================================
 * 배차 관리 페이지이다.
 * 서버에서 대기 주문 목록을 조회하고,
 * 강제 배차 / 배차 취소 기능을 제공한다.
 *
 * ★ 서버 연동:
 *   CMD_ORDER_MONITOR (510) - 대기 주문 모니터링
 *   CMD_RIDER_STATUS  (511) - 라이더 현황
 *   CMD_FORCE_DISPATCH(512) - 강제 배차
 *   CMD_FORCE_CANCEL  (513) - 배차 강제 취소
 * ============================================================
 */
class PageDispatch : public PageBase
{
    DECLARE_DYNAMIC(PageDispatch)

public:
    PageDispatch(CWnd* pParent = nullptr);
    virtual ~PageDispatch();

    // PageBase 인터페이스
    virtual void LoadData() override;
    virtual void SaveData() override;
    virtual CString GetPageName() const override { return _T("배차 관리"); }

    // 강제 배차 요청 (CMD_FORCE_DISPATCH = 512)
    void RequestForceDispatch(int orderId, int riderId);

    // 배차 강제 취소 요청 (CMD_FORCE_CANCEL = 513)
    void RequestForceCancel(int orderId);

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 메시지 핸들러
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBtnDispatchDel();
    afx_msg void OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    // 초기화 / 레이아웃
    void InitListCtrl();
    void RepositionList();

    // 서버에서 주문 모니터링 데이터 로드 (CMD_ORDER_MONITOR = 510)
    void LoadDataFromServer();

    // 컨트롤
    CListCtrl   m_listDispatch;
    CImageList  m_imgList;
    CFont       m_font;
};