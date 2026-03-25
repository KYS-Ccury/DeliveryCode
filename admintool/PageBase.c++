#pragma once

#include "pch.h"

// 공통 페이지 베이스 클래스
class CPageBase : public CDialogEx
{
    DECLARE_DYNAMIC(CPageBase)

public:
    // 생성자
    CPageBase(UINT nIDTemplate, CWnd* pParent = nullptr)
        : CDialogEx(nIDTemplate, pParent)
    {
    }

    // 소멸자
    virtual ~CPageBase()
    {
    }

    // 각 페이지가 자신의 리소스 ID를 반환하도록 강제
    virtual UINT GetDialogTemplateId() const = 0;

protected:
    virtual void DoDataExchange(CDataExchange* pDX)
    {
        CDialogEx::DoDataExchange(pDX);
    }

    DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPageBase, CDialogEx)
BEGIN_MESSAGE_MAP(CPageBase, CDialogEx)
END_MESSAGE_MAP()