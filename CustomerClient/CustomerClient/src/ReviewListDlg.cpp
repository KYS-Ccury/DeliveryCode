// ================================================================
//  ReviewListDlg.cpp  ─  리뷰 목록 화면 완성본
//
//  [연동 프로토콜]
//  ▶ 리뷰 목록 조회
//    REQ (207): { "store_id":101 }
//    RES: { "status":2000,
//           "avg_rating":4.2, "total":15,
//           "reviews":[
//             { "review_id":1, "author_id":"user01",
//               "rating":5, "content":"맛있어요!",
//               "created_at":"2024-05-22" }, ...
//           ]}
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ReviewListDlg.h"
#include "ReviewWriteDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

#define WM_REVIEW_LIST_RESPONSE (WM_USER + 175)

static std::string RLJStr(const std::string& j, const std::string& k)
{
    std::string t="\""+k+"\":\""; auto p=j.find(t);
    if(p==std::string::npos) return "";
    p+=t.size(); auto e=j.find('"',p);
    return (e==std::string::npos)?"":j.substr(p,e-p);
}
static int RLJInt(const std::string& j, const std::string& k)
{
    std::string t="\""+k+"\":"; auto p=j.find(t);
    if(p==std::string::npos) return 0;
    try{return std::stoi(j.substr(p+t.size()));}catch(...){return 0;}
}
static double RLJDouble(const std::string& j, const std::string& k)
{
    std::string t="\""+k+"\":"; auto p=j.find(t);
    if(p==std::string::npos) return 0.0;
    try{return std::stod(j.substr(p+t.size()));}catch(...){return 0.0;}
}
static std::vector<std::string> RLExtractObjs(const std::string& j, const std::string& arrKey)
{
    std::vector<std::string> res;
    std::string t="\""+arrKey+"\":["; auto ap=j.find(t);
    if(ap==std::string::npos) return res;
    size_t i=ap+t.size();
    while(i<j.size()){
        auto s=j.find('{',i); if(s==std::string::npos) break;
        int d=0; size_t e=s;
        for(;e<j.size();++e){ if(j[e]=='{')++d; else if(j[e]=='}'){ if(--d==0) break; } }
        res.push_back(j.substr(s,e-s+1)); i=e+1;
    }
    return res;
}

IMPLEMENT_DYNAMIC(ReviewListDlg, CDialogEx)
ReviewListDlg::ReviewListDlg(CWnd* pParent)
    : CDialogEx(IDD_REVIEW_LIST_DLG, pParent) {}
ReviewListDlg::~ReviewListDlg() {}

void ReviewListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_REVIEWS, m_listReviews);
}

BEGIN_MESSAGE_MAP(ReviewListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,            &ReviewListDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_WRITE_MY_REVIEW, &ReviewListDlg::OnBnClickedWriteMyReview)
    ON_MESSAGE(WM_REVIEW_LIST_RESPONSE,    &ReviewListDlg::OnReviewListResponse)
END_MESSAGE_MAP()

BOOL ReviewListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 리스트 컬럼 설정
    m_listReviews.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listReviews.InsertColumn(0, _T("별점"), LVCFMT_CENTER,  72);
    m_listReviews.InsertColumn(1, _T("내용"), LVCFMT_LEFT,   200);
    m_listReviews.InsertColumn(2, _T("작성자"),LVCFMT_CENTER, 62);
    m_listReviews.InsertColumn(3, _T("날짜"),  LVCFMT_CENTER, 70);

    // 가게 이름 표시
    CWnd* pStore = this->GetDlgItem(IDC_STATIC_REVIEW_STORE);
    if (pStore) pStore->SetWindowText(m_strStoreName);

    // 리뷰 작성 버튼: 구매 이력 있을 때만 활성화
    CWnd* pWrite = this->GetDlgItem(IDC_BTN_WRITE_MY_REVIEW);
    if (pWrite) pWrite->EnableWindow(m_bCanWriteReview);

    // 서버 콜백 등록
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_REVIEW_LIST,
        [hThis](uint16_t, const std::string& body) {
            std::string* pB = new std::string(body);
            ::PostMessage(hThis, WM_REVIEW_LIST_RESPONSE, 0, (LPARAM)pB);
        });

    m_listReviews.InsertItem(0, _T("리뷰를 불러오는 중..."));
    LoadReviews();
    return TRUE;
}

void ReviewListDlg::LoadReviews()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인 더미 데이터
        ReviewItem dummy1; dummy1.reviewID=1; dummy1.authorID="user01";
        dummy1.rating=5; dummy1.content="정말 맛있어요! 또 시킬게요."; dummy1.createdAt="2024-05-20";
        ReviewItem dummy2; dummy2.reviewID=2; dummy2.authorID="user02";
        dummy2.rating=4; dummy2.content="배달도 빠르고 음식도 맛있습니다."; dummy2.createdAt="2024-05-19";
        ReviewItem dummy3; dummy3.reviewID=3; dummy3.authorID="user03";
        dummy3.rating=3; dummy3.content="보통이에요. 그냥 무난합니다."; dummy3.createdAt="2024-05-18";
        m_vecReviews = { dummy1, dummy2, dummy3 };
        RebuildUI();
        return;
    }
    std::string json = "{\"store_id\":" + std::to_string(m_nStoreID) + "}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_REVIEW_LIST, json);
}

LRESULT ReviewListDlg::OnReviewListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (RLJInt(*pBody, "status") == (int)Status::SUCCESS) {
        m_vecReviews.clear();

        double avgRating = RLJDouble(*pBody, "avg_rating");
        int    total     = RLJInt(*pBody, "total");

        // 평균 별점 표시
        CWnd* pAvg = this->GetDlgItem(IDC_STATIC_AVG_RATING);
        if (pAvg) {
            CString s; s.Format(_T("%.1f점  (%d개)"), avgRating, total);
            pAvg->SetWindowText(s);
        }

        auto objs = RLExtractObjs(*pBody, "reviews");
        for (const auto& obj : objs) {
            ReviewItem rv;
            rv.reviewID  = RLJInt(obj, "review_id");
            rv.authorID  = RLJStr(obj, "author_id");
            rv.rating    = RLJInt(obj, "rating");
            rv.content   = RLJStr(obj, "content");
            rv.createdAt = RLJStr(obj, "created_at");
            m_vecReviews.push_back(rv);
        }
        RebuildUI();
    } else {
        m_listReviews.DeleteAllItems();
        m_listReviews.InsertItem(0, _T("리뷰 정보를 가져오지 못했습니다."));
    }
    delete pBody;
    return 0;
}

// 별점 숫자 → 별 문자열
CString ReviewListDlg::StarStr(int rating)
{
    CString s;
    for (int i = 0; i < 5; ++i)
        s += (i < rating) ? _T("★") : _T("☆");
    return s;
}

void ReviewListDlg::RebuildUI()
{
    m_listReviews.DeleteAllItems();
    if (m_vecReviews.empty()) {
        m_listReviews.InsertItem(0, _T("아직 리뷰가 없습니다."));
        return;
    }
    for (int i = 0; i < (int)m_vecReviews.size(); ++i) {
        const ReviewItem& rv = m_vecReviews[i];
        CString strStar  = StarStr(rv.rating);
        CString strCont  = CA2T(rv.content.c_str(),   CP_UTF8);
        CString strAuthor= CA2T(rv.authorID.c_str(),  CP_UTF8);
        CString strDate  = CA2T(rv.createdAt.c_str(), CP_UTF8);

        // 작성자 마스킹 (앞 2글자 + ***)
        if (strAuthor.GetLength() > 2)
            strAuthor = strAuthor.Left(2) + _T("***");

        int r = m_listReviews.InsertItem(i, strStar);
        m_listReviews.SetItemText(r, 1, strCont);
        m_listReviews.SetItemText(r, 2, strAuthor);
        m_listReviews.SetItemText(r, 3, strDate);
    }
}

void ReviewListDlg::OnBnClickedWriteMyReview()
{
    ReviewWriteDlg dlg(this);
    if (dlg.DoModal() == IDOK) {
        AfxMessageBox(_T("리뷰가 등록되었습니다!"), MB_ICONINFORMATION);
        // 리뷰 작성 후 목록 새로고침
        m_listReviews.DeleteAllItems();
        m_listReviews.InsertItem(0, _T("새로 고침 중..."));
        LoadReviews();
        // 중복 방지: 한 번만 작성 가능
        CWnd* pWrite = this->GetDlgItem(IDC_BTN_WRITE_MY_REVIEW);
        if (pWrite) pWrite->EnableWindow(FALSE);
    }
}

void ReviewListDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_REVIEW_LIST);
    CDialogEx::OnCancel();
}
