#pragma once

#include <afxdialogex.h>

/**
 * PageBase.h
 * ============================================================
 * 모든 페이지의 공통 베이스 클래스이다.
 * CDialogEx를 상속하며, 페이지 표시/숨김 및 공통 초기화를 제공한다.
 * ============================================================
 */
class PageBase : public CDialogEx
{
    DECLARE_DYNAMIC(PageBase)

public:
    // 생성자 / 소멸자
    PageBase(UINT nIDTemplate, CWnd* pParent = nullptr);
    virtual ~PageBase();

    // 페이지 표시 / 숨김
    void ShowPage();
    void HidePage();

    // 자식 페이지가 override 해야 할 인터페이스
    virtual void LoadData() {}
    virtual void SaveData() {}

    // 페이지 이름 (제목 표시용, 자식에서 override)
    virtual CString GetPageName() const { return _T(""); }

protected:
    // 초기화
    virtual BOOL OnInitDialog() override;

    // 리사이즈
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()
};