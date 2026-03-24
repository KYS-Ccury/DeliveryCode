// ================================================================
//  PaymentDlg.cpp  ─  결제 수단 관리 화면 (최종 완전판)
//
//  [컴파일 오류 원인 및 수정]
//  1. OnProfileResponse / OnAddCardResponse → h에 선언 추가
//  2. m_vecCards / m_bWaiting / m_str* → h에 멤버 선언 추가
//  3. RebuildCardListUI → h에 private 선언 추가
//  4. GetDlgItem(IDC)->EnableWindow() → GetDlgItem(IDC)->EnableWindow()
//     MFC에서 GetDlgItem은 CWnd* 반환 → ->EnableWindow() 정상
//     단, 전역 ::GetDlgItem(HWND, IDC) 와 혼동 주의
//     → CDialogEx 멤버 함수 GetDlgItem(IDC) 사용 (this-> 명시)
//  5. PostMessage(WM, W, L) → this->PostMessage() 로 명시
//  6. 람다 내 this 캡처 → [this] 로 명시
//
//  [IDC 정의]
//  resource.h 에 없는 IDC는 아래에서 임시 정의.
//  RC 편집기로 컨트롤 생성 후 resource.h에 자동 추가되면 이 블록 제거.
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "PaymentDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

// ── resource.h 에 없는 IDC 임시 정의 ─────────────────────────
#ifndef IDC_EDIT_CARD_NAME
#define IDC_EDIT_CARD_NAME    1910
#define IDC_EDIT_CARD_NUMBER  1911
#define IDC_EDIT_EXPIRY       1912
#define IDC_EDIT_CVV          1913
#define IDC_EDIT_PASSWORD     1914
#define IDC_LIST_CARDS        1915
#define IDC_BTN_SET_DEFAULT   1916
#define IDC_BTN_DELETE_CARD   1917
#endif

// ── 커스텀 윈도우 메시지 ──────────────────────────────────────
#define WM_PROFILE_RESPONSE (WM_USER + 140)
#define WM_ADDCARD_RESPONSE (WM_USER + 141)

// ── 유효성 검사 / 마스킹 헬퍼 ────────────────────────────────
static CString MaskCardNumber(const CString& raw)
{
    CString result = raw;
    for (int i = 0; i < result.GetLength() && i < 14; ++i)
        if (result[i] != '-') result.SetAt(i, '*');
    return result;
}

static bool ValidateCardFormat(const CString& num)
{
    CString clean;
    for (int i = 0; i < num.GetLength(); ++i)
        if (num[i] != '-') clean += num[i];
    if (clean.GetLength() != 16) return false;
    for (int i = 0; i < clean.GetLength(); ++i)
        if (!_istdigit(clean[i])) return false;
    return true;
}

static bool ValidateExpiry(const CString& expiry)
{
    if (expiry.GetLength() != 5 || expiry[2] != '/') return false;
    int month = _ttoi(expiry.Left(2));
    return (month >= 1 && month <= 12);
}

// ── JSON 파싱 헬퍼 ────────────────────────────────────────────
static std::string PJStr(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return "";
    pos += token.size();
    auto end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}

static int PJInt(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return 0;
    try { return std::stoi(json.substr(pos + token.size())); } catch (...) { return 0; }
}

static bool PJBool(const std::string& json, const std::string& key)
{
    std::string token = "\"" + key + "\":";
    auto pos = json.find(token);
    if (pos == std::string::npos) return false;
    return json.substr(pos + token.size(), 4) == "true";
}

// =================================================================

IMPLEMENT_DYNAMIC(PaymentDlg, CDialogEx)

PaymentDlg::PaymentDlg(CWnd* pParent)
    : CDialogEx(IDD_PAYMENT_DLG, pParent)
{}

PaymentDlg::~PaymentDlg() {}

void PaymentDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    // RC에 에디트 컨트롤 ID가 부여된 뒤 아래 주석 해제:
    // DDX_Text(pDX, IDC_EDIT_CARD_NAME,   m_strCardName);
    // DDX_Text(pDX, IDC_EDIT_CARD_NUMBER, m_strCardNumber);
    // DDX_Text(pDX, IDC_EDIT_EXPIRY,      m_strExpiry);
    // DDX_Text(pDX, IDC_EDIT_CVV,         m_strCVV);
    // DDX_Text(pDX, IDC_EDIT_PASSWORD,    m_strPassword);
}

BEGIN_MESSAGE_MAP(PaymentDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,                  &PaymentDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL,              &PaymentDlg::OnBnClickedCancel)
    ON_MESSAGE(WM_PROFILE_RESPONSE,      &PaymentDlg::OnProfileResponse)
    ON_MESSAGE(WM_ADDCARD_RESPONSE,      &PaymentDlg::OnAddCardResponse)
END_MESSAGE_MAP()

// ── 초기화 ────────────────────────────────────────────────────
BOOL PaymentDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // IDC_LIST_CARDS 컨트롤이 RC에 있으면 컬럼 초기화
    CWnd* pListWnd = this->GetDlgItem(IDC_LIST_CARDS);
    if (pListWnd) {
        CListCtrl* pList = static_cast<CListCtrl*>(pListWnd);
        pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        pList->InsertColumn(0, _T("별칭"),     LVCFMT_LEFT,  120);
        pList->InsertColumn(1, _T("카드번호"), LVCFMT_LEFT,  160);
        pList->InsertColumn(2, _T("기본"),     LVCFMT_CENTER, 50);
    }

    // 서버 응답 콜백 등록 (카드 목록 조회용)
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pBody);
        });

    // 기존 카드 목록 서버 요청
    if (net.IsConnected()) {
        std::string json = "{\"request_type\":\"payment_methods\"}";
        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCommon::REQ_GET_PROFILE, json);
    }

    return TRUE;
}

// ── 에디트 필드 읽기 (DDX 없이 안전하게 읽기) ────────────────
void PaymentDlg::ReadInputFields()
{
    auto ReadWnd = [&](int nID, CString& out) {
        CWnd* p = this->GetDlgItem(nID);
        if (p) p->GetWindowText(out);
    };
    ReadWnd(IDC_EDIT_CARD_NAME,   m_strCardName);
    ReadWnd(IDC_EDIT_CARD_NUMBER, m_strCardNumber);
    ReadWnd(IDC_EDIT_EXPIRY,      m_strExpiry);
    ReadWnd(IDC_EDIT_CVV,         m_strCVV);
    ReadWnd(IDC_EDIT_PASSWORD,    m_strPassword);
}

// ── 유효성 검사 ───────────────────────────────────────────────
bool PaymentDlg::ValidateInputs()
{
    ReadInputFields();

    if (m_strCardName.IsEmpty()) {
        AfxMessageBox(_T("카드 소유자 이름을 입력하세요."), MB_ICONWARNING);
        return false;
    }
    if (!ValidateCardFormat(m_strCardNumber)) {
        AfxMessageBox(_T("카드 번호 16자리를 올바르게 입력하세요.\n예: 1234-5678-9012-3456"),
                      MB_ICONWARNING);
        return false;
    }
    if (!ValidateExpiry(m_strExpiry)) {
        AfxMessageBox(_T("유효기간을 MM/YY 형식으로 입력하세요.\n예: 12/27"), MB_ICONWARNING);
        return false;
    }
    if (m_strCVV.GetLength() < 3) {
        AfxMessageBox(_T("보안코드(CVV) 3자리를 입력하세요."), MB_ICONWARNING);
        return false;
    }
    if (m_strPassword.GetLength() < 2) {
        AfxMessageBox(_T("비밀번호 앞 2자리를 입력하세요."), MB_ICONWARNING);
        return false;
    }
    return true;
}

// ── 카드 목록 UI 갱신 ────────────────────────────────────────
void PaymentDlg::RebuildCardListUI()
{
    CWnd* pListWnd = this->GetDlgItem(IDC_LIST_CARDS);
    if (!pListWnd) return;

    CListCtrl* pList = static_cast<CListCtrl*>(pListWnd);
    pList->DeleteAllItems();

    for (int i = 0; i < (int)m_vecCards.size(); ++i) {
        int nRow = pList->InsertItem(i, m_vecCards[i].alias);
        pList->SetItemText(nRow, 1, m_vecCards[i].masked);
        pList->SetItemText(nRow, 2, m_vecCards[i].isDefault ? _T("V") : _T(""));
    }
    if (m_vecCards.empty())
        pList->InsertItem(0, _T("등록된 카드가 없습니다."));
}

// ── 카드 목록 서버 응답 (WM_PROFILE_RESPONSE) ─────────────────
LRESULT PaymentDlg::OnProfileResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    int status = PJInt(*pBody, "status");
    if (status == (int)Status::SUCCESS) {
        m_vecCards.clear();

        // "payment_methods":[{...},{...}] 파싱
        std::string arrToken = "\"payment_methods\":[";
        auto arrPos = pBody->find(arrToken);
        if (arrPos != std::string::npos) {
            size_t i = arrPos + arrToken.size();
            while (i < pBody->size()) {
                auto objStart = pBody->find('{', i);
                if (objStart == std::string::npos) break;
                int depth = 0;
                size_t objEnd = objStart;
                for (; objEnd < pBody->size(); ++objEnd) {
                    if ((*pBody)[objEnd] == '{') ++depth;
                    else if ((*pBody)[objEnd] == '}') { if (--depth == 0) break; }
                }
                std::string obj = pBody->substr(objStart, objEnd - objStart + 1);

                PaymentCard card;
                card.id        = PJInt(obj,  "id");
                card.alias     = CA2T(PJStr(obj, "alias").c_str(),       CP_UTF8);
                card.masked    = CA2T(PJStr(obj, "masked").c_str(),      CP_UTF8);
                card.type      = CA2T(PJStr(obj, "method_type").c_str(), CP_UTF8);
                card.isDefault = PJBool(obj, "is_default");
                if (card.id > 0) m_vecCards.push_back(card);

                i = objEnd + 1;
            }
        }
        RebuildCardListUI();
    }

    delete pBody;
    return 0;
}

// ── 카드 등록 버튼 (IDOK) ────────────────────────────────────
void PaymentDlg::OnBnClickedOk()
{
    if (m_bWaiting.load()) return;
    if (!ValidateInputs()) return;

    CString masked = MaskCardNumber(m_strCardNumber);

    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인: 로컬에만 추가
        PaymentCard card;
        card.id        = (int)m_vecCards.size() + 1;
        card.alias     = m_strCardName;
        card.masked    = masked;
        card.type      = _T("카드");
        card.isDefault = m_vecCards.empty();
        m_vecCards.push_back(card);
        RebuildCardListUI();
        AfxMessageBox(_T("카드가 임시 등록되었습니다. (서버 미연결)"), MB_ICONINFORMATION);
        return;
    }

    // 서버 전송 (마스킹값만 전송 — 실제 카드번호 비전송)
    std::string alias     = CT2A(m_strCardName, CP_UTF8);
    std::string maskedStr = CT2A(masked,         CP_UTF8);
    std::string json =
        "{\"request_type\":\"add_card\","
        "\"card_alias\":\""      + alias     + "\","
        "\"card_num_masked\":\"" + maskedStr + "\","
        "\"method_type\":\"카드\"}";

    // 등록 응답 콜백으로 교체
    net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            this->PostMessage(WM_ADDCARD_RESPONSE, 0, (LPARAM)pBody);
        });

    m_bWaiting.store(true);
    this->GetDlgItem(IDOK)->EnableWindow(FALSE);
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_GET_PROFILE, json);
}

// ── 카드 등록 서버 응답 (WM_ADDCARD_RESPONSE) ────────────────
LRESULT PaymentDlg::OnAddCardResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);

    m_bWaiting.store(false);
    this->GetDlgItem(IDOK)->EnableWindow(TRUE);

    if (pBody) {
        int status = PJInt(*pBody, "status");
        delete pBody;

        if (status == (int)Status::SUCCESS) {
            AfxMessageBox(_T("카드가 등록되었습니다."), MB_ICONINFORMATION);

            // 카드 목록 재조회 콜백 재등록
            auto& net = NetworkManager::GetInstance();
            if (net.IsConnected()) {
                net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
                    [this](uint16_t, const std::string& body) {
                        std::string* pb = new std::string(body);
                        this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pb);
                    });
                std::string json2 = "{\"request_type\":\"payment_methods\"}";
                net.SendPacket((uint8_t)ClientType::CUSTOMER,
                               CmdCommon::REQ_GET_PROFILE, json2);
            }

            // 입력 필드 초기화
            m_strCardName = m_strCardNumber = m_strExpiry =
            m_strCVV      = m_strPassword   = _T("");
            UpdateData(FALSE);
        } else {
            AfxMessageBox(_T("카드 등록에 실패했습니다."), MB_ICONERROR);
        }
    }
    return 0;
}

// ── 취소 버튼 ─────────────────────────────────────────────────
void PaymentDlg::OnBnClickedCancel()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCommon::REQ_GET_PROFILE);
    CDialogEx::OnCancel();
}
