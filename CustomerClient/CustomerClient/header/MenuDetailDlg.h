#pragma once
#include "afxdialogex.h"
#include "MenuInfo.h"
#include "CartItem.h"
//#include "MyInfoDlg.h"


// ================================================================
//  MenuDetailDlg.h  ─  메뉴 상세 / 옵션 선택 / 장바구니 담기
//
//  [변경사항]
//  - m_nQuantity 수량 멤버 추가
//  - 수량 +/- 버튼 핸들러 추가
//  - 옵션 체크 변경 → 합계 자동 갱신 핸들러 추가
//  - UpdateTotal() 헬퍼 추가
// ================================================================
class MenuDetailDlg : public CDialogEx
{
    DECLARE_DYNAMIC(MenuDetailDlg)
public:
    MenuDetailDlg(CWnd* pParent = nullptr);
    virtual ~MenuDetailDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MENUDETAIL_DLG };
#endif
    CString  m_strMenuName;
    MenuInfo m_menuInfo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    afx_msg void OnBnClickedBtnBack();
    afx_msg void OnBnClickedBtnCart();
    afx_msg void InsertOptionData();

    // ── 수량 버튼 ────────────────────────────────────────────
    afx_msg void OnBnClickedCountMinus();
    afx_msg void OnBnClickedCountPlus();

    // ── 옵션 체크 변경 → 합계 갱신 ──────────────────────────
    afx_msg void OnLvnItemchangedOptions(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnPaint();      // 음식 이미지 그리기

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl m_listOptions;
    int       m_nQuantity;       // 주문 수량 (기본 1)
    HBITMAP   m_hMenuImg = nullptr; // 음식 이미지 비트맵

    void UpdateTotal();          // 옵션 + 수량 기반 합계 갱신
    void LoadMenuImage();        // 서버에서 이미지 로드
};
