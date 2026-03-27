#pragma once
#include "afxdialogex.h"

class CSalesDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSalesDlg)

public:
	CSalesDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CSalesDlg();

#ifdef AFX_DESIGN_TIME
	// [체크] 따옴표를 지웠습니다!
	enum { IDD = IDD_SALES_MGR_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

	// 👇 여기서부터 추가 👇
public:
	virtual BOOL OnInitDialog(); // 창 켜질 때 실행될 함수
	afx_msg void OnBnClickedBtnSearchSales(); // [조회] 버튼 클릭 함수

	afx_msg void OnBnClickedMonthBtn(UINT nID); // 1~12월 통합 처리 함수
	afx_msg void OnBnClickedBtnExportExcel();   // 엑셀 변환 버튼 함수
};