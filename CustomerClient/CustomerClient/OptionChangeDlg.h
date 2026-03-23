#pragma once
#include "afxdialogex.h"


// OptionChangeDlg 대화 상자

class OptionChangeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(OptionChangeDlg)

public:
	OptionChangeDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~OptionChangeDlg();

	// 외부(장바구니)에서 넘겨받을 데이터
	CString m_strMenuName;
	int m_nQuantity;
	int m_nBasePrice; // 기본 가격을 받을 변수 추가
	CListCtrl m_listOptions; // 이 줄을 추가하세요!

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_OPTION_CHANGE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnCountMinus(); // 마이너스 버튼 클릭 핸들러
	afx_msg void OnBnClickedBtnCountPlus();  // 플러스 버튼 클릭 핸들러
	afx_msg void OnLvnItemchangedListOptions(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

public:
	void UpdateTotal(); // 합계를 계산해서 UI에 뿌려주는 함수
};
