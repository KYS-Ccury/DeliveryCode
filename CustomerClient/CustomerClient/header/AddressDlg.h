#pragma once
// ================================================================
//  AddressDlg.h  — 배달 주소 관리 다이얼로그
//
//  IDD_ADDRESS_DLG (2400) 를 CustomerClient.rc 에 추가해야 함
//  (아래 RC 스니펫 참고)
//
//  resource.h 에 추가 필요:
//    #define IDD_ADDRESS_DLG    2400
//    #define IDC_ADDR_LIST      2401
//    #define IDC_ADDR_EDIT      2402
//    #define IDC_ADDR_ADD       2403
//    #define IDC_ADDR_DELETE    2404
//    #define IDC_ADDR_DEFAULT   2405
//    #define IDC_ADDR_SELECT    2406
// ================================================================
#include "afxdialogex.h"
#include <string>

class AddressDlg : public CDialogEx
{
    DECLARE_DYNAMIC(AddressDlg)
public:
    explicit AddressDlg(CWnd* pParent = nullptr);
    virtual ~AddressDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ADDRESS_DLG };
#endif

    // 컨트롤 ID (resource.h 에 정의됨)
    // IDC_ADDR_LIST    2401
    // IDC_ADDR_EDIT    2402
    // IDC_ADDR_ADD     2403
    // IDC_ADDR_DELETE  2404
    // IDC_ADDR_DEFAULT 2405
    // IDC_ADDR_SELECT  2406

    CString m_strSelectedAddr;   // DoModal 후 선택된 주소

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK()     override;
    virtual void OnCancel() override;

    afx_msg void OnBtnAdd();
    afx_msg void OnBtnDelete();
    afx_msg void OnBtnSetDefault();
    afx_msg void OnBtnSelect();

    DECLARE_MESSAGE_MAP()

private:
    CListBox m_listAddr;
    CEdit    m_editNewAddr;

    void RefreshList();
    int  GetSelectedIndex() const;
};
