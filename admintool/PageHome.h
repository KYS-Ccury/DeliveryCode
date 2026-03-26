#pragma once
#include "PageBase.h"
#include "resource.h"
#include <functional>
#include <cmath>

// 홈 대시보드 페이지이다.
// 좌측 상단 파이차트, 우측 상단 배차 현황, 하단 문의 리스트를 가진다.
class PageHome : public PageBase
{
    DECLARE_DYNAMIC(PageHome)

public:
    // 생성자이다.
    PageHome(CWnd* pParent = nullptr);

    // 소멸자이다.
    virtual ~PageHome();

    enum { IDD = IDD_PAGE_HOME };

    // 페이지 이름을 반환한다.
    virtual CString GetPageName() override { return _T("Home"); }

    // 홈 데이터 갱신 함수이다.
    virtual void LoadData() override;

    // 홈 저장 함수이다.
    virtual void SaveData() override {}

    // 상단 카운트값을 외부에서 설정한다.
    void SetCounts(int nReview, int nDispatch, int nInquiry);

    // 문의 리스트 클릭 시 다른 페이지로 넘기기 위한 콜백이다.
    std::function<void()> m_fnGoInquiry;

    // 리뷰 차트 클릭 시 리뷰 페이지로 넘기기 위한 콜백이다.
    std::function<void()> m_fnGoReview;

    // 배차 리스트 클릭 시 배차 페이지로 넘기기 위한 콜백이다.
    std::function<void()> m_fnGoDispatch;

protected:
    // 우측 상단 배차 리스트이다.
    CListCtrl  m_listDispatch;

    // 하단 문의 리스트이다.
    CListCtrl  m_listInquiry;

    // 공통 폰트이다.
    CFont      m_font;

    // 리스트용 이미지리스트이다.
    CImageList m_imgDispatch;
    CImageList m_imgInquiry;

    // 표시용 개수 값들이다.
    int m_nReviewCount;
    int m_nDispatchCount;
    int m_nInquiryCount;

    // 현재 마우스 hover 중인 파이 조각 인덱스이다.
    int m_nHoverSlice;

    // 레이아웃 마진이다.
    static const int MARGIN = 8;

    // 동적 생성 리스트 컨트롤 ID이다.
    static const int DISPATCH_ID = 6002;
    static const int INQUIRY_ID = 6003;

    // 파이차트 한 조각 정보이다.
    struct SliceInfo
    {
        double   startAngle;
        double   endAngle;
        COLORREF color;
        int      count;
    };

    // 파이 조각 정보 배열이다.
    SliceInfo m_slices[5];

    // 차트가 그려질 사각형 영역이다.
    CRect m_rcChart;

    // DDX 함수이다.
    virtual void DoDataExchange(CDataExchange* pDX) override;

    // 초기화 함수이다.
    virtual BOOL OnInitDialog() override;

    // 배차 리스트 초기화이다.
    void InitDispatchList();

    // 문의 리스트 초기화이다.
    void InitInquiryList();

    // 배차 더미 데이터를 넣는다.
    void InsertDispatchDummy();

    // 문의 더미 데이터를 넣는다.
    void InsertInquiryDummy();

    // 현재 크기에 맞게 레이아웃을 다시 계산한다.
    void UpdateLayout();

    // 파이차트 조각 데이터를 구성한다.
    void BuildSlices();

    // 파이차트를 실제로 그린다.
    void DrawPieChart(CDC* pDC, const CRect& rc);

    // 각도를 0 ~ 2π 범위로 정규화한다.
    double NormalizeAngle(double dAngle) const;

    // 차트 배경을 그린다.
    void DrawChartBackground(CDC* pDC, const CRect& rc);

    // 리사이즈 처리이다.
    afx_msg void OnSize(UINT nType, int cx, int cy);

    // 페인트 처리이다.
    afx_msg void OnPaint();

    // 배경 지우기 처리이다.
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    // 마우스 이동 처리이다.
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);

    // 마우스 왼쪽 클릭 처리이다 (차트 영역 → 리뷰 페이지 이동).
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    // 문의 리스트 클릭 처리이다.
    afx_msg void OnInquiryClick(NMHDR* pNMHDR, LRESULT* pResult);

    // 배차 리스트 클릭 처리이다.
    afx_msg void OnDispatchClick(NMHDR* pNMHDR, LRESULT* pResult);

    // 배차 리스트 상태 컬러링 처리이다.
    afx_msg void OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
};