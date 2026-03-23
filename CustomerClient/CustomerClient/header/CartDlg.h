#pragma once
#include <vector>
#include "afxdialogex.h"

#include "CartItem.h" 

// CartDlg

//// 장바구니 아이템 정보를 담는 구조체
//struct CartItem {
//	CString menuName;
//	int price;
//	int quantity;
//	// ... 추가 옵션 정보들
//};

class CartDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CartDlg)

public:
	CartDlg(CWnd* pParent = nullptr);
	virtual ~CartDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CART_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV

	virtual BOOL OnInitDialog(); // 'CartDlg::' 제거 및 virtual 추가

	afx_msg void OnBnClickedOk(); // 주문완료창
	afx_msg void OnBnClickedBtnBack(); // 뒤로가기
	afx_msg void OnBnClickedBtnEditOption();   // 옵션변경 함수 선언

public:
	std::vector<CartItem> m_vecCart; // 현재 장바구니에 담긴 아이템들
	CListCtrl m_listCart;            // 리소스 뷰에서 만든 리스트 컨트롤 연결 변수

	void UpdateCartUI();             // UI를 데이터에 맞춰 새로고침하는 함수

	DECLARE_MESSAGE_MAP()
};
