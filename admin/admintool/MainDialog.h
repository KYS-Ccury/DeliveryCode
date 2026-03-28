#pragma once
#include <afxdialogex.h>
#include "resource.h"
#include "PageBase.h"
#include "PageHome.h"
#include "PageReview.h"
#include "PageDispatch.h"
#include "PageInquiry.h"

// 메인 관리자 다이얼로그이다.
// 좌측 메뉴, 상단 제목, 중앙 페이지 교체 영역을 관리한다.
//
// ★ 수정: 폴링 시작/종료, WM_POLL_xxx 메시지 핸들러 추가
class CMainDialog : public CDialogEx
{
public:
    CMainDialog(CWnd* pParent = nullptr);
    virtual ~CMainDialog();

    enum { IDD = IDD_MAIN_DIALOG };

protected:
    CStatic m_staticPage;
    CStatic m_staticTitle;

    PageHome* m_pPageHome;
    PageReview* m_pPageReview;
    PageDispatch* m_pPageDispatch;
    PageInquiry* m_pPageInquiry;

    PageBase* m_pCurrentPage;

    CArray<PageBase*, PageBase*> m_pageHistory;

    HICON m_hIcon;
    CFont m_font;

    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    void CreatePages();
    void SwitchPage(PageBase* pNewPage, bool bSaveHistory = true);
    void ResizePageToArea(PageBase* pPage);
    void UpdateTitle(const CString& strTitle);
    CRect GetPageAreaClientRect() const;

    afx_msg void OnBtnMenuReview();
    afx_msg void OnBtnMenuDispatch();
    afx_msg void OnBtnMenuInquiry();
    afx_msg void OnBtnHome();
    afx_msg void OnBtnBack();
    afx_msg void OnBtnSave();
    afx_msg void OnClose();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

    // ★ 폴링 결과 수신 핸들러
    afx_msg LRESULT OnPollHeartbeatOk(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnPollHeartbeatFail(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnPollNewMessages(WPARAM wParam, LPARAM lParam);

    virtual void OnOK() override {}
    virtual void OnCancel() override {}

    DECLARE_MESSAGE_MAP()
};