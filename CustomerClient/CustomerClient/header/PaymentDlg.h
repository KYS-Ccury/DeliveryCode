#pragma once
#include "afxdialogex.h"
#include <vector>
#include <atomic>

// ================================================================
//  PaymentDlg.h  ─  결제 수단 관리 화면 (최종 완전판)
//
//  ※ PaymentCard 는 이 헤더에서만 정의 (cpp 재정의 금지)
//  ※ 멤버 함수/변수가 h/cpp 완전히 일치
// ================================================================

struct PaymentCard {
    int     id        = 0;
    CString alias;
    CString masked;
    CString type;
    bool    isDefault = false;
};

class PaymentDlg : public CDialogEx
{
    DECLARE_DYNAMIC(PaymentDlg)
public:
    PaymentDlg(CWnd* pParent = nullptr);
    virtual ~PaymentDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PAYMENT_DLG };
#endif

    // ── 입력 필드 (DDX 또는 GetDlgItemText 로 채워짐) ────────
    CString m_strCardName;
    CString m_strCardNumber;
    CString m_strExpiry;
    CString m_strCVV;
    CString m_strPassword;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedCancel();

    // ── 서버 응답 핸들러 ─────────────────────────────────────
    afx_msg LRESULT OnProfileResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddCardResponse(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // ── 데이터 ───────────────────────────────────────────────
    std::vector<PaymentCard> m_vecCards;
    std::atomic<bool>        m_bWaiting{ false };

    // ── 내부 헬퍼 ────────────────────────────────────────────
    void RebuildCardListUI();
    bool ValidateInputs();
    void ReadInputFields();   // GetDlgItemText 로 m_str* 채우기
};
