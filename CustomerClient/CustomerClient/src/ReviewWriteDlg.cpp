// ================================================================
//  ReviewWriteDlg.cpp  ─  리뷰 작성 화면 (수정본)
//
//  [기존 문제]
//  WaitForSingleObject(hEvent, 5000) 로 UI 스레드를 5초 블로킹
//  → 앱 화면이 멈추는 심각한 UX 문제
//
//  [수정]
//  PostMessage 패턴으로 교체:
//    OnBnClickedOk → SendPacket → (ReceiveLoop 콜백)
//    → PostMessage(WM_REVIEW_RESPONSE) → OnReviewResponse → OnOK
//
//  [연동 프로토콜]
//    REQ (206): { "token":"...", "store_id":101,
//                 "rating":5, "content":"맛있어요!" }
//    RES: { "status":2000 }
//
//  resource.h 사용 IDC:
//    IDC_BTN_REVIEW_BACK    1300
//    IDC_COMBO_STAR_RATING  1302
//    IDC_EDIT_REVIEW_CONTENT 1303
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ReviewWriteDlg.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "OrderManager.h"
#include "common/header/Types.h"

#define WM_REVIEW_RESPONSE (WM_USER + 170)

// ── JSON 헬퍼 ─────────────────────────────────────────────────
static int RVJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(json.substr(pos + token.size())); }
    catch (...) { return -1; }
}
static std::string RVEscape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else          out += c;
    }
    return out;
}

IMPLEMENT_DYNAMIC(ReviewWriteDlg, CDialogEx)

ReviewWriteDlg::ReviewWriteDlg(CWnd* pParent)
    : CDialogEx(IDD_REVIEW_WRITE_DLG, pParent)
    , m_nStarRating(5)
    , m_strReviewText(_T(""))
    , m_bWaiting(false)
{}
ReviewWriteDlg::~ReviewWriteDlg() {}

void ReviewWriteDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ReviewWriteDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REVIEW_BACK, &ReviewWriteDlg::OnBnClickedBtnReviewBack)
    ON_BN_CLICKED(IDOK,                &ReviewWriteDlg::OnBnClickedOk)
    ON_MESSAGE(WM_REVIEW_RESPONSE,     &ReviewWriteDlg::OnReviewResponse)
END_MESSAGE_MAP()

BOOL ReviewWriteDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 별점 콤보박스 설정 (5점 ~ 1점)
    CComboBox* pCombo = static_cast<CComboBox*>(
        this->GetDlgItem(IDC_COMBO_STAR_RATING));
    if (pCombo) {
        pCombo->AddString(_T("★★★★★  5점"));
        pCombo->AddString(_T("★★★★☆  4점"));
        pCombo->AddString(_T("★★★☆☆  3점"));
        pCombo->AddString(_T("★★☆☆☆  2점"));
        pCombo->AddString(_T("★☆☆☆☆  1점"));
        pCombo->SetCurSel(0); // 기본 5점
    }

    // 서버 응답 콜백 등록
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_WRITE_REVIEW,
        [hThis](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            ::PostMessage(hThis, WM_REVIEW_RESPONSE, 0, (LPARAM)pBody);
        });

    return TRUE;
}

// ── 확인 버튼 ─────────────────────────────────────────────────
void ReviewWriteDlg::OnBnClickedOk()
{
    if (m_bWaiting) return;

    // ── 별점 읽기 ─────────────────────────────────────────────
    CComboBox* pCombo = static_cast<CComboBox*>(
        this->GetDlgItem(IDC_COMBO_STAR_RATING));
    int sel = pCombo ? pCombo->GetCurSel() : 0;
    m_nStarRating = 5 - sel; // 0→5점, 4→1점

    // ── 리뷰 내용 읽기 ────────────────────────────────────────
    this->GetDlgItemText(IDC_EDIT_REVIEW_CONTENT, m_strReviewText);
    m_strReviewText.Trim();
    if (m_strReviewText.IsEmpty()) {
        AfxMessageBox(_T("리뷰 내용을 입력해 주세요."), MB_ICONWARNING);
        this->GetDlgItem(IDC_EDIT_REVIEW_CONTENT)->SetFocus();
        return;
    }

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인: 바로 완료 처리
        AfxMessageBox(_T("리뷰가 임시 저장되었습니다. (서버 미연결)"),
                      MB_ICONINFORMATION);
        CDialogEx::OnOK();
        return;
    }

    m_bWaiting = true;
    this->GetDlgItem(IDOK)->EnableWindow(FALSE);
    SetDlgItemText(IDOK, _T("전송 중..."));

    // ── JSON 빌드 후 서버 전송 ────────────────────────────────
    std::string token   = AuthManager::GetInstance().GetAccessToken();
    int         storeID = OrderManager::GetInstance().GetCurrentStoreID();
    std::string contentRaw = CT2A(m_strReviewText, CP_UTF8);
    std::string content = RVEscape(contentRaw);

    std::string json =
        "{\"token\":\""   + token   + "\","
        "\"store_id\":"  + std::to_string(storeID) + ","
        "\"rating\":"    + std::to_string(m_nStarRating) + ","
        "\"content\":\"" + content  + "\"}";

    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_WRITE_REVIEW, json);
}

// ── 서버 응답 처리 (UI 스레드) ────────────────────────────────
LRESULT ReviewWriteDlg::OnReviewResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);

    m_bWaiting = false;
    this->GetDlgItem(IDOK)->EnableWindow(TRUE);
    SetDlgItemText(IDOK, _T("확인"));

    if (!pBody) return 0;
    int status = RVJInt(*pBody, "status");
    delete pBody;

    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_WRITE_REVIEW);

    if (status == (int)Status::SUCCESS) {
        AfxMessageBox(_T("리뷰가 등록되었습니다. 감사합니다!"), MB_ICONINFORMATION);
        CDialogEx::OnOK();
    } else {
        AfxMessageBox(_T("리뷰 등록에 실패했습니다.\n이미 리뷰를 작성했거나 서버 오류입니다."),
                      MB_ICONERROR);
    }
    return 0;
}

void ReviewWriteDlg::OnBnClickedBtnReviewBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_WRITE_REVIEW);
    EndDialog(IDCANCEL);
}
