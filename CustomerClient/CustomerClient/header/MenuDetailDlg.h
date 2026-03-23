#pragma once
#include "afxdialogex.h"


// MenuDetailDlg ダイアログ

class MenuDetailDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MenuDetailDlg)

public:
	MenuDetailDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~MenuDetailDlg();

	CString m_strMenuName; // 음식 이름을 받을 변수
	CListCtrl m_listOptions; // 리스트 컨트롤 변수 선언

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IDD_MENUDETAIL_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	afx_msg void OnBnClickedBtnBack();
	afx_msg void OnBnClickedBtnCart();
	virtual void OnOK(); // IDOK(담기) 재정의

	afx_msg BOOL OnInitDialog();
	afx_msg void InsertOptionData();

	DECLARE_MESSAGE_MAP()
};
