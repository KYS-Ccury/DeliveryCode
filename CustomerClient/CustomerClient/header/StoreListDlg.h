#pragma once
#include "afxdialogex.h"

#include "ScrollMenu.h" // 스크롤 메뉴 클래스

// StoreListDlg

class StoreListDlg : public CDialogEx
{
	DECLARE_DYNAMIC(StoreListDlg)

public:
	StoreListDlg(CWnd* pParent = nullptr);
	virtual ~StoreListDlg();


#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STORELIST_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 서포트
	virtual BOOL OnInitDialog(); // 초기화 함수
    afx_msg void OnBnClickedBtnCart(); // 장바구니
    afx_msg void OnBnClickedBtnStoreInfo(); // 매장상세화면
    afx_msg void OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult); // 옵션선택화면

	DECLARE_MESSAGE_MAP()

public:
    // 컨트롤 변수
    CListCtrl m_listMenu;           // 음식 리스트 (IDC_LIST_MENU_ITEMS)
    CScrollMenu m_wndScrollMenu;    // 상단 음식 카테고리 스크롤바 (IDC_STATIC_SUB_MENU_BAR)

    // 데이터 보관용
    std::vector<CString> m_vecSubCategories;

    // 핸들러 함수
    afx_msg void OnBnClickedBtnBack();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt); // 휠 스크롤 연동
    afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam); // 메뉴 클릭 처리

    void UpdateMenuListUI(CString subCategory); // 음식 리스트 갱신 함수
};
