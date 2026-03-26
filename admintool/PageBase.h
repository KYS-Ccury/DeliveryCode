#pragma once
#include <afxdialogex.h>

// 공통 페이지 베이스 클래스이다.
// 모든 하위 페이지가 같은 방식으로 표시/숨김/기본 동작을 공유하도록 한다.
class PageBase : public CDialogEx
{
    DECLARE_DYNAMIC(PageBase)

public:
    // 페이지 템플릿 ID와 부모 윈도우를 전달받아 생성한다.
    PageBase(UINT nIDTemplate, CWnd* pParent = nullptr);

    // 가상 소멸자이다.
    virtual ~PageBase();

    // 페이지를 표시한다.
    virtual void ShowPage();

    // 페이지를 숨긴다.
    virtual void HidePage();

    // 페이지 진입 시 데이터 갱신용 함수이다.
    virtual void LoadData() {}

    // 페이지 저장용 함수이다.
    virtual void SaveData() {}

    // 페이지 제목 표시용 이름을 반환한다.
    virtual CString GetPageName() { return _T(""); }

protected:
    // 공통 초기화 함수이다.
    virtual BOOL OnInitDialog() override;

    // 공통 리사이즈 처리 함수이다.
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()
};