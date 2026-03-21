#pragma once
#include "afxdialogex.h"

#include <vector>


// MainHomeDlg 대화 상자

class MainHomeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MainHomeDlg)

public:
	MainHomeDlg(CWnd* pParent = nullptr);   // 표준 콘스트럭트
	virtual ~MainHomeDlg();

// 디어로그 데이터
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAINHOME_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원
	virtual BOOL OnInitDialog();

	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

	int m_nTargetX;      // 최종적으로 도달해야 할 목표 X 좌표 (첫 번째 버튼 기준)
	int m_nStepX;        // 한 프레임당 이동할 거리
public:
	CListCtrl m_listStore;

	// MFC에서는 동적으로 생성한 버튼(new로 만든 버튼)들을 나중에 제어(위치 이동, 삭제 등)하기 위해
	// 그 버튼들의주소값(포인터)을 저장해둘 보관함이 필요
	std::vector<CButton*> m_vCatButtons; // 버튼 포인터들을 담을 벡터
	CBrush m_brushWhite; // 창 전체 배경용
	CBrush m_brushBack; // 배경색 브러쉬

	// 드래그 상태 관리용 변수
	bool    m_bDragging = false;  // 초기값 false 필수
	CPoint  m_ptLastMouse;        // 마우스 위치 저장



};


