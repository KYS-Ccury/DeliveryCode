#pragma once
#include "CClientSocket.h" 


class COwnerDlg : public CDialogEx
{

public:
	COwnerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	CFont m_fontLogo;
	bool m_bIsOpen;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OWNER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnCustomdrawListOrder(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnNewOrderReceived(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent); // 🚨 [추가] 타이머 메시지 처리기
	afx_msg void OnBnClickedBtnPrintReceipt();
	afx_msg void OnBnClickedBtnPrintDelivery();

	DECLARE_MESSAGE_MAP()
public:
	CClientSocket m_NotifySocket; // 🚨 24시간 대기할 알림용 소켓
	afx_msg void OnBnClickedBtnOrderMgr();  // 기존에 있던 것
	afx_msg void OnBnClickedBtnStoreMgr();  // 추가
	afx_msg void OnBnClickedBtnSalesMgr();  // 추가
	afx_msg void OnBnClickedBtnSettings();   // 추가
	afx_msg void OnBnClickedBtnStatus();     // 추가
	afx_msg void OnNMClickListOrder(NMHDR* pNMHDR, LRESULT* pResult);
	int m_nLastOrderCount; // 이전 주문 개수 기억용
	void RefreshOrderListSilently(); // 백그라운드 새로고침 함수

	afx_msg void OnBnClickedBtnTimePlus();
	afx_msg void OnBnClickedBtnTimeMinus();
	afx_msg void OnBnClickedBtnOrderAccept();
	afx_msg void OnBnClickedBtnOrderReject();
	afx_msg void OnBnClickedBtnInquiry();
bool m_bIsListOpen;

private:
	CRect m_rcOriginals[30]; // 상세 영역 컨트롤들의 원래 위치 저장용
	bool m_bCoordsInitialized = false; // 위치 저장 여부 체크
};
