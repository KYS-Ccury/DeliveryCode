#pragma once
// ================================================================
//  AddressDlg.h  — 배달 주소 관리 다이얼로그
//
//  [수정] 다이얼로그 열릴 때 서버에서 직접 주소 목록을 받아옴
//         WM_ADDR_REFRESH (WM_USER+120) 메시지로 비동기 갱신
// ================================================================
#include "afxdialogex.h"
#include <string>

// 서버 응답 수신 후 목록 갱신 트리거 메시지
#define WM_ADDR_REFRESH  (WM_USER + 120)

class AddressDlg : public CDialogEx
{
    DECLARE_DYNAMIC(AddressDlg)
public:
    explicit AddressDlg(CWnd* pParent = nullptr);
    virtual ~AddressDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ADDRESS_DLG };
#endif

    CString m_strSelectedAddr;   // DoModal 후 선택된 주소

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK()     override;
    virtual void OnCancel() override;

    afx_msg void    OnBtnAdd();
    afx_msg void    OnBtnDelete();
    afx_msg void    OnBtnSetDefault();
    afx_msg void    OnBtnSelect();
    // ★ 서버 응답 수신 메시지 핸들러
    afx_msg LRESULT OnAddrRefresh(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CListBox m_listAddr;
    CEdit    m_editNewAddr;

    void RefreshList();
    int  GetSelectedIndex() const;

    // ★ 서버에 주소 목록 요청 + 콜백 등록
    void RequestAndRefresh();
};
