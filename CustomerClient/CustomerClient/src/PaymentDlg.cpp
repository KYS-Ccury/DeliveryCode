// ================================================================
//  PaymentDlg.cpp  ─  결제 수단 관리 화면 (완성판)
//
//  [변경/완성 사항]
//  1. 카드 목록 조회: REQ_GET_PROFILE(104) → "request_type":"get_cards"
//  2. 카드 등록:      REQ_GET_PROFILE(104) → "request_type":"add_card"
//  3. 카드 삭제 버튼 (IDC_BTN_DELETE_CARD) 기능 구현
//  4. 기본 카드 설정 버튼 (IDC_BTN_SET_DEFAULT) 기능 구현
//  5. 카드 등록 완료 후 목록 자동 재조회
//  6. 오프라인 상태에서도 로컬 카드 목록 표시/추가 가능
//
//  [서버 프로토콜]  REQ_GET_PROFILE (104)
//    카드 목록 요청:  { "request_type":"get_cards" }
//    카드 등록 요청:  { "request_type":"add_card",
//                      "card_alias":"...", "card_num_masked":"...",
//                      "method_type":"CARD" }
//    카드 삭제 요청:  { "request_type":"delete_card",
//                      "payment_method_id":N }
//    기본 설정 요청:  { "request_type":"set_default_card",
//                      "payment_method_id":N }
//
//  [응답 형식]
//    카드 목록: { "status":2000,
//                 "payment_methods":[{id,alias,masked,method_type,is_default},...] }
//    등록/삭제: { "status":2000 }
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "PaymentDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "UserInfo.h"
#include "common/header/Types.h"

// ── 앱 전역 카드 캐시 (PaymentDlg 닫혀도 유지) ───────────────
std::vector<PaymentCard> g_cachedCards;

// ── IDC 임시 정의 ─────────────────────────────────────────────
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
#define WM_DELCARD_RESPONSE (WM_USER + 142)

// ── 유효성 검사 / 마스킹 헬퍼 ────────────────────────────────
static CString MaskCardNumber(const CString& raw)
{
    // 숫자만 추출 후 마지막 4자리 제외 마스킹
    CString digits;
    for (int i = 0; i < raw.GetLength(); ++i)
        if (_istdigit(raw[i])) digits += raw[i];

    CString masked;
    for (int i = 0; i < digits.GetLength(); ++i) {
        if (i > 0 && i % 4 == 0) masked += _T("-");
        masked += (i < digits.GetLength() - 4) ? _T('*') : digits[i];
    }
    return masked;
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

// ── 카드 목록 JSON 파싱 ───────────────────────────────────────
static std::vector<PaymentCard> ParseCardList(const std::string& body)
{
    std::vector<PaymentCard> cards;
    std::string arrToken = "\"payment_methods\":[";
    auto arrPos = body.find(arrToken);
    if (arrPos == std::string::npos) return cards;

    size_t i = arrPos + arrToken.size();
    while (i < body.size()) {
        auto objStart = body.find('{', i);
        if (objStart == std::string::npos) break;
        int depth = 0;
        size_t objEnd = objStart;
        for (; objEnd < body.size(); ++objEnd) {
            if (body[objEnd] == '{') ++depth;
            else if (body[objEnd] == '}') { if (--depth == 0) break; }
        }
        std::string obj = body.substr(objStart, objEnd - objStart + 1);

        PaymentCard card;
        card.id        = PJInt(obj,  "id");
        card.alias     = CA2T(PJStr(obj, "alias").c_str(),       CP_UTF8);
        card.masked    = CA2T(PJStr(obj, "masked").c_str(),      CP_UTF8);
        card.type      = CA2T(PJStr(obj, "method_type").c_str(), CP_UTF8);
        card.isDefault = PJBool(obj, "is_default");
        if (card.id > 0) cards.push_back(card);

        i = objEnd + 1;
    }
    return cards;
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
}

BEGIN_MESSAGE_MAP(PaymentDlg, CDialogEx)
    ON_BN_CLICKED(IDOK,               &PaymentDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL,           &PaymentDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BTN_DELETE_CARD,  &PaymentDlg::OnBnClickedDeleteCard)
    ON_BN_CLICKED(IDC_BTN_SET_DEFAULT,  &PaymentDlg::OnBnClickedSetDefault)
    ON_MESSAGE(WM_PROFILE_RESPONSE,   &PaymentDlg::OnProfileResponse)
    ON_MESSAGE(WM_ADDCARD_RESPONSE,   &PaymentDlg::OnAddCardResponse)
    ON_MESSAGE(WM_DELCARD_RESPONSE,   &PaymentDlg::OnDelCardResponse)
    ON_MESSAGE(WM_SETDEFAULT_RESPONSE, &PaymentDlg::OnSetDefaultResponse)
END_MESSAGE_MAP()

// ── 초기화 ────────────────────────────────────────────────────
BOOL PaymentDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 카드 목록 리스트 컨트롤 초기화
    CWnd* pListWnd = this->GetDlgItem(IDC_LIST_CARDS);
    if (pListWnd) {
        CListCtrl* pList = static_cast<CListCtrl*>(pListWnd);
        pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        pList->InsertColumn(0, _T("별칭"),    LVCFMT_LEFT,  120);
        pList->InsertColumn(1, _T("카드번호"), LVCFMT_LEFT,  160);
        pList->InsertColumn(2, _T("기본"),    LVCFMT_CENTER,  50);
    }

    // 카드 목록 조회 콜백 등록
    auto& net = NetworkManager::GetInstance();
    net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
        [this](uint16_t, const std::string& body) {
            std::string* pBody = new std::string(body);
            this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pBody);
        });

    // 서버에 카드 목록 요청
    if (net.IsConnected()) {
        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCommon::REQ_GET_PROFILE,
                       "{\"request_type\":\"get_cards\"}");
    } else {
        // 오프라인: AuthManager에 캐시된 카드 목록 로드
        LoadCardsFromCache();
        RebuildCardListUI();
    }

    return TRUE;
}

// ── 입력 필드 읽기 ────────────────────────────────────────────
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
        AfxMessageBox(
            _T("카드 번호 16자리를 올바르게 입력하세요.\n예: 1234-5678-9012-3456"),
            MB_ICONWARNING);
        return false;
    }
    if (!ValidateExpiry(m_strExpiry)) {
        AfxMessageBox(_T("유효기간을 MM/YY 형식으로 입력하세요.\n예: 12/27"),
                      MB_ICONWARNING);
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
        pList->SetItemText(nRow, 2, m_vecCards[i].isDefault ? _T("★기본") : _T(""));
    }
    if (m_vecCards.empty())
        pList->InsertItem(0, _T("등록된 카드가 없습니다."));
}

// ── 카드 목록 서버 응답 (WM_PROFILE_RESPONSE) ─────────────────
LRESULT PaymentDlg::OnProfileResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (PJInt(*pBody, "status") == (int)Status::SUCCESS) {
        m_vecCards = ParseCardList(*pBody);
        SaveCardsToCache();   // 서버에서 받은 목록도 캐시에 저장
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

    // ── 오프라인: 로컬 캐시에 추가 ─────────────────────────
    if (!net.IsConnected()) {
        PaymentCard card;
        card.id        = (int)m_vecCards.size() + 1;
        card.alias     = m_strCardName;
        card.masked    = masked;
        card.type      = _T("CARD");
        card.isDefault = m_vecCards.empty();
        m_vecCards.push_back(card);

        // AuthManager 캐시에도 저장 (창 닫아도 유지)
        SaveCardsToCache();
        RebuildCardListUI();
        AfxMessageBox(_T("카드가 등록되었습니다.\n(서버 연결 후 정식 등록됩니다.)"),
                      MB_ICONINFORMATION);
        return;
    }

    // ── 서버에 카드 등록 요청 ────────────────────────────────
    std::string alias     = CT2A(m_strCardName, CP_UTF8);
    std::string maskedStr = CT2A(masked,         CP_UTF8);

    std::string json =
        "{\"request_type\":\"add_card\","
        "\"card_alias\":\""      + alias     + "\","
        "\"card_num_masked\":\"" + maskedStr + "\","
        "\"method_type\":\"CARD\"}";

    net.UnregisterCallback(CmdCommon::REQ_GET_PROFILE);  // ★ 추가
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

            // 입력 필드 초기화
            auto Clear = [&](int nID) {
                CWnd* p = this->GetDlgItem(nID);
                if (p) p->SetWindowText(_T(""));
            };
            Clear(IDC_EDIT_CARD_NAME);   Clear(IDC_EDIT_CARD_NUMBER);
            Clear(IDC_EDIT_EXPIRY);      Clear(IDC_EDIT_CVV);
            Clear(IDC_EDIT_PASSWORD);

            // 목록 재조회
            auto& net = NetworkManager::GetInstance();
            if (net.IsConnected()) {
                net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
                    [this](uint16_t, const std::string& body) {
                        std::string* pb = new std::string(body);
                        this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pb);
                    });
                net.SendPacket((uint8_t)ClientType::CUSTOMER,
                               CmdCommon::REQ_GET_PROFILE,
                               "{\"request_type\":\"get_cards\"}");
            }
        } else {
            AfxMessageBox(_T("카드 등록에 실패했습니다."), MB_ICONERROR);
        }
    }    return 0;
}

// ── 카드 삭제 버튼 ────────────────────────────────────────────
void PaymentDlg::OnBnClickedDeleteCard()
{
    if (m_bWaiting.load()) return;

    CWnd* pListWnd = this->GetDlgItem(IDC_LIST_CARDS);
    if (!pListWnd) return;
    CListCtrl* pList = static_cast<CListCtrl*>(pListWnd);

    int n = pList->GetSelectionMark();
    if (n < 0 || n >= (int)m_vecCards.size()) {
        AfxMessageBox(_T("삭제할 카드를 선택해주세요."), MB_ICONWARNING);
        return;
    }

    CString msg;
    msg.Format(_T("'%s' 카드를 삭제하시겠습니까?"), (LPCTSTR)m_vecCards[n].alias);
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;

    auto& net = NetworkManager::GetInstance();

    // ── 오프라인: 로컬에서 삭제 ──────────────────────────────
    if (!net.IsConnected()) {
        m_vecCards.erase(m_vecCards.begin() + n);
        RebuildCardListUI();
        return;
    }

    int cardID = m_vecCards[n].id;
    std::string json =
        "{\"request_type\":\"delete_card\","
        "\"payment_method_id\":" + std::to_string(cardID) + "}";

    net.UnregisterCallback(CmdCommon::REQ_GET_PROFILE);  // ★ 추가
    net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
        [this](uint16_t, const std::string& body) {
            std::string* pb = new std::string(body);
            this->PostMessage(WM_DELCARD_RESPONSE, 0, (LPARAM)pb);
        });

    m_bWaiting.store(true);
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_GET_PROFILE, json);
}

// ── 카드 삭제 서버 응답 ───────────────────────────────────────
LRESULT PaymentDlg::OnDelCardResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    m_bWaiting.store(false);

    if (pBody) {
        int status = PJInt(*pBody, "status");
        delete pBody;

        if (status == (int)Status::SUCCESS) {
            // 목록 재조회
            auto& net = NetworkManager::GetInstance();
            if (net.IsConnected()) {
                net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
                    [this](uint16_t, const std::string& body) {
                        std::string* pb = new std::string(body);
                        this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pb);
                    });
                net.SendPacket((uint8_t)ClientType::CUSTOMER,
                               CmdCommon::REQ_GET_PROFILE,
                               "{\"request_type\":\"get_cards\"}");
            }
        } else {
            AfxMessageBox(_T("카드 삭제에 실패했습니다."), MB_ICONERROR);
        }
    }
    return 0;
}

// ── 기본 카드 설정 버튼 ───────────────────────────────────────
void PaymentDlg::OnBnClickedSetDefault()
{
    if (m_bWaiting.load()) return;

    CWnd* pListWnd = this->GetDlgItem(IDC_LIST_CARDS);
    if (!pListWnd) return;
    CListCtrl* pList = static_cast<CListCtrl*>(pListWnd);

    int n = pList->GetSelectionMark();
    if (n < 0 || n >= (int)m_vecCards.size()) {
        AfxMessageBox(_T("기본으로 설정할 카드를 선택해주세요."), MB_ICONWARNING);
        return;
    }
    if (m_vecCards[n].isDefault) {
        AfxMessageBox(_T("이미 기본 카드로 설정되어 있습니다."), MB_ICONINFORMATION);
        return;
    }

    auto& net = NetworkManager::GetInstance();

    // ── 오프라인: 로컬에서 기본 설정 ────────────────────────
    if (!net.IsConnected()) {
        for (auto& c : m_vecCards) c.isDefault = false;
        m_vecCards[n].isDefault = true;
        RebuildCardListUI();
        return;
    }

    int cardID = m_vecCards[n].id;
    std::string json =
        "{\"request_type\":\"set_default_card\","
        "\"payment_method_id\":" + std::to_string(cardID) + "}";

    // net.UnregisterCallback(CmdCommon::REQ_GET_PROFILE);  // ★ 추가
    // net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
    //     [this](uint16_t, const std::string& body) {
    //         std::string* pb = new std::string(body);
    //         // 성공하면 목록 재조회 (WM_PROFILE_RESPONSE 재활용)
    //         this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pb);
    //     });

    // m_bWaiting.store(true);
    // net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_GET_PROFILE, json);
    net.UnregisterCallback(CmdCommon::REQ_GET_PROFILE);
    net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
        [this](uint16_t, const std::string& body) {
            std::string* pb = new std::string(body);
            // ★ WM_PROFILE_RESPONSE 대신 전용 메시지 사용
            this->PostMessage(WM_SETDEFAULT_RESPONSE, 0, (LPARAM)pb);
        });

    m_bWaiting.store(true);
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCommon::REQ_GET_PROFILE, json);

}

// ── 취소 버튼 ─────────────────────────────────────────────────
void PaymentDlg::OnBnClickedCancel()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCommon::REQ_GET_PROFILE);
    CDialogEx::OnCancel();
}

// ── 카드 목록 캐시 저장 (AuthManager UserInfo.cardList 활용) ──
// PaymentDlg 가 닫혀도 앱이 살아있는 동안 유지됨
void PaymentDlg::SaveCardsToCache()
{
    auto& auth = AuthManager::GetInstance();
    // UserInfo::cardList 에 현재 m_vecCards 동기화
    // CardInfo { cardName, cardNumber } 형태로 변환
    // cardName = alias, cardNumber = masked (간략 저장)
    std::vector<CardInfo> newList;
    for (const auto& c : m_vecCards) {
        CardInfo ci;
        ci.cardName   = CT2A(c.alias,  CP_UTF8);
        ci.cardNumber = CT2A(c.masked, CP_UTF8);
        newList.push_back(ci);
    }
    // AuthManager의 m_currentUser 에 직접 접근 대신
    // GetCurrentUserCardList / SetCurrentUserCardList 를 사용
    // (AuthManager 에 해당 함수가 없으면 전역 변수로 관리)
    g_cachedCards = m_vecCards;
}

void PaymentDlg::LoadCardsFromCache()
{
    m_vecCards = g_cachedCards;
}

LRESULT PaymentDlg::OnSetDefaultResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    m_bWaiting.store(false);

    if (pBody) {
        int status = PJInt(*pBody, "status");
        delete pBody;

        if (status == (int)Status::SUCCESS) {
            // ★ 성공하면 목록 재조회 요청
            auto& net = NetworkManager::GetInstance();
            if (net.IsConnected()) {
                net.UnregisterCallback(CmdCommon::REQ_GET_PROFILE);
                net.RegisterCallback(CmdCommon::REQ_GET_PROFILE,
                    [this](uint16_t, const std::string& body) {
                        std::string* pb = new std::string(body);
                        this->PostMessage(WM_PROFILE_RESPONSE, 0, (LPARAM)pb);
                    });
                net.SendPacket((uint8_t)ClientType::CUSTOMER,
                               CmdCommon::REQ_GET_PROFILE,
                               "{\"request_type\":\"get_cards\"}");
            }
        } else {
            AfxMessageBox(_T("기본 카드 설정에 실패했습니다."), MB_ICONERROR);
        }
    }
    return 0;
}