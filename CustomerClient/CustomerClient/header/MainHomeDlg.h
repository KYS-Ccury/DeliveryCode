#pragma once                     // 헤더 중복 포함 방지
#include "afxdialogex.h"         // MFC 확장 대화 상자 클래스 헤더
#include <vector>                // 동적 배열(std::vector) 사용을 위한 헤더
#include "ScrollMenu.h"          // 커스텀 컨트롤인 ScrollMenu 클래스 헤더


// MainHomeDlg 대화 상자 클래스 선언
class MainHomeDlg : public CDialogEx
{
	// 이 클래스가 실행 중에 자신의 타입 정보를 알 수 있게 함 (IMPLEMENT_DYNAMIC과 짝꿍)
	DECLARE_DYNAMIC(MainHomeDlg)

public:
	// ─────────────────────────────────────────────
	//  생성자 및 소멸자
	// ─────────────────────────────────────────────
	MainHomeDlg(CWnd* pParent = nullptr);   // 표준 생성자 (부모 윈도우 지정 가능)
	virtual ~MainHomeDlg();                 // 소멸자 (메모리 해제 시 호출)

// 대화 상자 데이터 (리소스 ID 연결)
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAINHOME_DLG };        // 이 클래스가 사용할 도면(IDD) 번호
#endif

protected:
	// ─────────────────────────────────────────────
	//  상속받은 주요 가상 함수 (Override)
	// ─────────────────────────────────────────────
	// UI 컨트롤과 변수를 연결해주는 데이터 교환 함수
	virtual void DoDataExchange(CDataExchange* pDX);

	// 대화 상자가 나타나기 전 초기 설정을 수행하는 함수
	virtual BOOL OnInitDialog();

	// 음식카테고리메뉴에 따라 출력되는 매장정보를 가져와 출력
	void UpdateStoreListUI(CString categoryName);

	// ─────────────────────────────────────────────
	//  메시지 핸들러 함수 (이벤트 처리)
	// ─────────────────────────────────────────────
	// afx_msg: 이 함수가 메시지 맵에 연결될 '메시지 처리용'임을 표시

	// 마우스 휠 조작 시 호출되는 함수
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	// 컨트롤이나 배경의 색상을 결정하기 위해 호출되는 함수
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	// 사용자가 커스텀 메뉴 버튼을 클릭했을 때 호출되는 사용자 정의 메시지 함수
	afx_msg LRESULT OnScrollMenuClicked(WPARAM wParam, LPARAM lParam);

	// 장바구니 버튼 (ID IDC_BUTTON1)
	// 현재 주문완료창이 뜨게 해 놨으나 추후 변경 필요
	afx_msg void OnBnClickedButton1();

	// 매장리스트에서 매장클릭
	afx_msg void OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnBnClickedBtnOrderHistory();

	// 메시지 맵을 사용하겠다고 선언 (BEGIN_MESSAGE_MAP과 짝꿍)
	DECLARE_MESSAGE_MAP()


public:
	// ─────────────────────────────────────────────
	//  UI 멤버 변수 (컨트롤 및 그래픽 객체)
	// ─────────────────────────────────────────────

	// 음식카테고리메뉴 목록
	std::vector<CString> m_vecCategories;

	// IDC_LIST_STOR: 가게 목록을 보여주는 리스트 컨트롤 변수
	CListCtrl m_listStore;

	// 화면을 하얀색으로 깨끗하게 밀어버릴 때 사용할 페인트 붓
	CBrush m_brushWhite;

	// 배달 앱 특유의 연회색(통로색)을 칠할 때 사용할 페인트 붓
	CBrush m_brushBack;

	// IDC_STATIC_BG: 상단 카테고리 메뉴(전체, 치킨 등)를 관리하는 커스텀 컨트롤 객체
	CScrollMenu m_wndScrollMenu;
};


