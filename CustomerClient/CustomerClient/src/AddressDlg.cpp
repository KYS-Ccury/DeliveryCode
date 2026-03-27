// ================================================================
//  AddressDlg.cpp  — 배달 주소 관리 다이얼로그
//
//  [수정 핵심]
//  OnInitDialog() 에서 서버에 REQ_GET_ADDRESSES(213)를 직접 요청.
//  응답이 오면 WM_ADDR_REFRESH 메시지로 AddressManager를 갱신하고
//  목록을 다시 그림.
//
//  이렇게 하면 로그아웃 → 재로그인 후 AddressDlg를 열어도
//  항상 DB의 최신 주소 목록을 정확히 표시함.
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "AddressDlg.h"
#include "AddressManager.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

// ── 간이 JSON 파싱 헬퍼 ──────────────────────────────────────
static int ADJInt(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return -1;
    try { return std::stoi(j.substr(p + t.size())); } catch (...) { return -1; }
}

IMPLEMENT_DYNAMIC(AddressDlg, CDialogEx)

BEGIN_MESSAGE_MAP(AddressDlg, CDialogEx)
    ON_BN_CLICKED(IDC_ADDR_ADD,      &AddressDlg::OnBtnAdd)
    ON_BN_CLICKED(IDC_ADDR_DELETE,   &AddressDlg::OnBtnDelete)
    ON_BN_CLICKED(IDC_ADDR_DEFAULT,  &AddressDlg::OnBtnSetDefault)
    ON_BN_CLICKED(IDC_ADDR_SELECT,   &AddressDlg::OnBtnSelect)
    ON_BN_CLICKED(IDCANCEL,          &AddressDlg::OnCancel)
    // ★ 서버 응답 수신 후 목록 갱신
    ON_MESSAGE(WM_ADDR_REFRESH,      &AddressDlg::OnAddrRefresh)
END_MESSAGE_MAP()

AddressDlg::AddressDlg(CWnd* pParent)
    : CDialogEx(IDD_ADDRESS_DLG, pParent) {}
AddressDlg::~AddressDlg()
{
    // 다이얼로그가 닫힐 때 콜백 해제 (중복 수신 방지)
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_ADDRESSES);
}

void AddressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ADDR_LIST,  m_listAddr);
    DDX_Control(pDX, IDC_ADDR_EDIT,  m_editNewAddr);
}

// ================================================================
//  OnInitDialog
//  ★ 서버에 주소 목록을 직접 요청 → 응답 오면 OnAddrRefresh 호출
//     서버 미연결 시에는 AddressManager 메모리 데이터로 표시
// ================================================================
BOOL AddressDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ::SendMessage(m_editNewAddr.GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                  (LPARAM)_T("새 주소를 입력하세요 (예: 광주시 북구 용봉동)"));

    // 우선 현재 메모리 데이터로 빠르게 표시 (서버 응답 전 임시)
    RefreshList();

    // ★ 서버에서 최신 목록 요청 (비동기 갱신)
    RequestAndRefresh();

    return TRUE;
}

// ================================================================
//  RequestAndRefresh
//  ★ REQ_GET_ADDRESSES(213) 전송 + 응답 콜백 등록
//     콜백에서 WM_ADDR_REFRESH를 PostMessage → OnAddrRefresh 호출
// ================================================================
void AddressDlg::RequestAndRefresh()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;  // 오프라인: 메모리 데이터 그대로 사용

    HWND hThis = GetSafeHwnd();

    // 콜백 등록 — 응답이 오면 이 다이얼로그 윈도우에 메시지 전송
    net.RegisterCallback(CmdCustomer::REQ_GET_ADDRESSES,
        [hThis](uint16_t, const std::string& body) {
            // 워커 스레드에서 호출되므로 PostMessage로 UI 스레드에 전달
            std::string* pBody = new std::string(body);
            ::PostMessage(hThis, WM_ADDR_REFRESH, 0, (LPARAM)pBody);
        });

    // 서버에 주소 목록 요청
    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json  = "{\"token\":\"" + token + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_GET_ADDRESSES, json);
}

// ================================================================
//  OnAddrRefresh  —  WM_ADDR_REFRESH 수신 시 호출
//  ★ AddressManager에 서버 응답 반영 → 목록 재표시
// ================================================================
LRESULT AddressDlg::OnAddrRefresh(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (ADJInt(*pBody, "status") == (int)Status::SUCCESS) {
        // AddressManager 메모리를 서버 DB 데이터로 교체
        AddressManager::GetInstance().OnAddressListResponse(*pBody);
        // 목록 UI 갱신
        RefreshList();
    }

    // 콜백 해제 (1회성 갱신)
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_ADDRESSES);

    delete pBody;
    return 0;
}

// ================================================================
//  RefreshList  —  AddressManager 메모리 → ListBox 표시
// ================================================================
void AddressDlg::RefreshList()
{
    m_listAddr.ResetContent();
    const auto& list = AddressManager::GetInstance().GetAll();
    for (const auto& item : list) {
        CString entry = CA2T(item.addr.c_str(), CP_UTF8);
        if (item.isDefault) entry = _T("★ ") + entry;
        m_listAddr.AddString(entry);
    }
    int defIdx = AddressManager::GetInstance().GetDefaultIndex();
    if (defIdx >= 0) m_listAddr.SetCurSel(defIdx);
}

int AddressDlg::GetSelectedIndex() const { return m_listAddr.GetCurSel(); }

// ================================================================
//  버튼 핸들러들
//  AddressManager의 Add/Delete/SetDefault 내부에서 서버 전송 처리됨
// ================================================================
void AddressDlg::OnBtnAdd()
{
    CString s; m_editNewAddr.GetWindowText(s); s.Trim();
    if (s.IsEmpty()) {
        MessageBox(_T("주소를 입력하세요."), _T("알림"), MB_OK); return;
    }

    // ★ AddressManager::AddAddress → 서버 REQ_SAVE_ADDRESS(214) 자동 전송
    AddressManager::GetInstance().AddAddress(std::string(CT2A(s, CP_UTF8)));
    m_editNewAddr.SetWindowText(_T(""));
    RefreshList();
    m_listAddr.SetCurSel(AddressManager::GetInstance().Count() - 1);
}

void AddressDlg::OnBtnDelete()
{
    int idx = GetSelectedIndex();
    if (idx < 0) {
        MessageBox(_T("삭제할 주소를 선택하세요."), _T("알림"), MB_OK); return;
    }
    if (MessageBox(_T("선택한 주소를 삭제하시겠습니까?"), _T("확인"), MB_YESNO) != IDYES) return;

    // ★ AddressManager::DeleteAddress → 서버 REQ_DELETE_ADDRESS(215) 자동 전송
    AddressManager::GetInstance().DeleteAddress(idx);
    RefreshList();
}

void AddressDlg::OnBtnSetDefault()
{
    int idx = GetSelectedIndex();
    if (idx < 0) {
        MessageBox(_T("기본으로 설정할 주소를 선택하세요."), _T("알림"), MB_OK); return;
    }

    // ★ AddressManager::SetDefault → 서버 REQ_DEFAULT_ADDRESS(216) 자동 전송
    AddressManager::GetInstance().SetDefault(idx);
    RefreshList();
}

void AddressDlg::OnBtnSelect()
{
    int idx = GetSelectedIndex();
    if (idx < 0 || idx >= AddressManager::GetInstance().Count()) {
        EndDialog(IDCANCEL); return;
    }
    const auto& item = AddressManager::GetInstance().GetAll()[idx];
    m_strSelectedAddr = CA2T(item.addr.c_str(), CP_UTF8);

    // ★ 선택한 주소를 기본 주소로 설정 → 서버 REQ_DEFAULT_ADDRESS(216) 자동 전송
    AddressManager::GetInstance().SetDefault(idx);
    EndDialog(IDOK);
}

void AddressDlg::OnOK()     { OnBtnSelect(); }
void AddressDlg::OnCancel() { EndDialog(IDCANCEL); }
