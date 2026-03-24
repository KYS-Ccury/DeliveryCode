#pragma once
#include "afxdialogex.h"

// PaymentDlg : 카드 등록 화면
class PaymentDlg : public CDialogEx
{
    DECLARE_DYNAMIC(PaymentDlg)

public:
    PaymentDlg(CWnd* pParent = nullptr);
    virtual ~PaymentDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PAYMENT_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();

    DECLARE_MESSAGE_MAP()

public:
    // 입력 필드 멤버
    CString m_strCardName;    // 소유자 이름
    CString m_strCardNumber;  // 카드번호 (0000-0000-0000-0000)
    CString m_strExpiry;      // 유효기간 (MM/YY)
    CString m_strCVV;         // 보안코드
    CString m_strPassword;    // 비밀번호 앞 2자리
};
