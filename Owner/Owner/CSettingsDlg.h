#pragma once
#include "afxdialogex.h"

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);
	virtual ~CSettingsDlg();

#ifdef AFX_DESIGN_TIME
	// [체크] 쌍따옴표 제거!
	enum { IDD = IDD_SETTINGS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

	// 👇 추가 기능 👇
public:
	virtual BOOL OnInitDialog(); // 초기 세팅
	virtual void OnOK();         // 저장(IDOK) 버튼을 눌렀을 때
};