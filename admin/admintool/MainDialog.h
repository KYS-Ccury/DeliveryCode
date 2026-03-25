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
class CMainDialog : public CDialogEx
{
public:
    // 메인 다이얼로그 생성자이다.
    CMainDialog(CWnd* pParent = nullptr);

    // 메인 다이얼로그 소멸자이다.
    virtual ~CMainDialog();

    enum { IDD = IDD_MAIN_DIALOG };

protected:
    // 중앙 페이지가 들어갈 영역이다.
    CStatic m_staticPage;

    // 상단 제목 표시용 static이다.
    CStatic m_staticTitle;

    // 각 페이지 포인터이다.
    PageHome* m_pPageHome;
    PageReview* m_pPageReview;
    PageDispatch* m_pPageDispatch;
    PageInquiry* m_pPageInquiry;

    // 현재 표시 중인 페이지이다.
    PageBase* m_pCurrentPage;

    // 뒤로가기용 히스토리이다.
    CArray<PageBase*, PageBase*> m_pageHistory;

    // 다이얼로그 아이콘이다.
    HICON m_hIcon;

    // 제목 글꼴이다.
    CFont m_font;

    // DDX 바인딩 함수이다.
    virtual void DoDataExchange(CDataExchange* pDX) override;

    // 초기화 함수이다.
    virtual BOOL OnInitDialog() override;

    // 모든 페이지를 생성한다.
    void CreatePages();

    // 페이지를 전환한다.
    void SwitchPage(PageBase* pNewPage, bool bSaveHistory = true);

    // 특정 페이지를 중앙 영역에 맞게 리사이즈한다.
    void ResizePageToArea(PageBase* pPage);

    // 현재 페이지 제목을 갱신한다.
    void UpdateTitle(const CString& strTitle);

    // 현재 페이지 영역 크기를 구한다.
    CRect GetPageAreaClientRect() const;

    // 리뷰 메뉴 버튼 처리이다.
    afx_msg void OnBtnMenuReview();

    // 배차 메뉴 버튼 처리이다.
    afx_msg void OnBtnMenuDispatch();

    // 문의 메뉴 버튼 처리이다.
    afx_msg void OnBtnMenuInquiry();

    // 홈 버튼 처리이다.
    afx_msg void OnBtnHome();

    // 뒤로가기 버튼 처리이다.
    afx_msg void OnBtnBack();

    // 저장 버튼 처리이다.
    afx_msg void OnBtnSave();

    // 닫기 처리이다.
    afx_msg void OnClose();

    // 페인트 처리이다.
    afx_msg void OnPaint();

    // 아이콘 드래그 처리이다.
    afx_msg HCURSOR OnQueryDragIcon();

    // 엔터키 닫힘 방지이다.
    virtual void OnOK() override {}

    // ESC 닫힘 방지이다.
    virtual void OnCancel() override {}

    DECLARE_MESSAGE_MAP()
};