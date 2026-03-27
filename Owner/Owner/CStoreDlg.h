#pragma once
#include "afxdialogex.h"

class CStoreDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStoreDlg)

public:
	CStoreDlg(CWnd* pParent = nullptr);  
	virtual ~CStoreDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STORE_MGR_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog(); // 창 초기화 함수
	afx_msg void OnNMClickListStoreMenu(NMHDR* pNMHDR, LRESULT* pResult); // 리스트 클릭 이벤트
	
	afx_msg void OnBnClickedBtnAddMenu();

	afx_msg void OnBnClickedBtnEditMenu(); // 수정 버튼
	afx_msg void OnBnClickedBtnDelMenu(); // 삭제 버튼

	CString m_strSelectedImagePath; // 선택된 이미지 경로 저장용
	void DisplayImage(CString strPath); // 이미지를 화면에 그리는 함수
	afx_msg void OnBnClickedBtnSelectImage(); // 이미지 선택 버튼 클릭 이벤트
};
