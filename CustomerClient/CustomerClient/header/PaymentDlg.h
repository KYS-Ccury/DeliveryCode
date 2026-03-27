#pragma once
#include "afxdialogex.h"
#include <vector>
#include <atomic>

#define WM_SETDEFAULT_RESPONSE (WM_USER + 143)

// ================================================================
//  PaymentDlg.h  ─  결제 수단 관리 화면 (완성판)
//
//  [변경사항]
//  - OnBnClickedDeleteCard / OnBnClickedSetDefault 핸들러 추가
//  - OnDelCardResponse 응답 핸들러 추가
// ================================================================

struct PaymentCard {
    int     id        = 0;
    CString alias;
    CString masked;
    CString type;
    bool    isDefault = false;
};

// ── 앱 전역 카드 캐시 (PaymentDlg 닫혀도 유지, CartDlg에서 참조) ──
extern std::vector<PaymentCard> g_cachedCards;

class PaymentDlg : public CDialogEx
{
    DECLARE_DYNAMIC(PaymentDlg)
public:
    PaymentDlg(CWnd* pParent = nullptr);
    virtual ~PaymentDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PAYMENT_DLG };
#endif
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
    afx_msg void    OnBnClickedDeleteCard();   // ← 카드 삭제
    afx_msg void    OnBnClickedSetDefault();   // ← 기본 카드 설정

    afx_msg LRESULT OnProfileResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddCardResponse(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDelCardResponse(WPARAM wParam, LPARAM lParam); // ← 삭제 응답
    afx_msg LRESULT OnSetDefaultResponse(WPARAM, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    std::vector<PaymentCard> m_vecCards;
    std::atomic<bool>        m_bWaiting{ false };

    void RebuildCardListUI();
    bool ValidateInputs();
    void ReadInputFields();
    void SaveCardsToCache();    // 카드 목록을 전역 캐시에 저장
    void LoadCardsFromCache();  // 전역 캐시에서 카드 목록 복원
};
