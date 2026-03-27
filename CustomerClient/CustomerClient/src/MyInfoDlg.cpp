// ================================================================
//  MyInfoDlg.cpp  ─  개인정보 확인
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MyInfoDlg.h"
#include "NetworkManager.h"
#include "AuthManager.h"
#include "common/header/Types.h"

static std::string MIJStr(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":\"";
    auto p = j.find(t); if (p == std::string::npos) return "";
    p += t.size(); auto e = j.find('"', p);
    return (e == std::string::npos) ? "" : j.substr(p, e - p);
}
static int MIJInt(const std::string& j, const std::string& k)
{
    std::string t = "\"" + k + "\":";
    auto p = j.find(t); if (p == std::string::npos) return -1;
    try { return std::stoi(j.substr(p + t.size())); } catch (...) { return -1; }
}

IMPLEMENT_DYNAMIC(MyInfoDlg, CDialogEx)

MyInfoDlg::MyInfoDlg(CWnd* pParent)
    : CDialogEx(IDD_MYINFO_DLG, pParent)
{}

MyInfoDlg::~MyInfoDlg() {}

void MyInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(MyInfoDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,     &MyInfoDlg::OnBnClickedBtnBack)
    ON_MESSAGE(WM_MYINFO_RESPONSE,  &MyInfoDlg::OnMyInfoResponse)
END_MESSAGE_MAP()

BOOL MyInfoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 모든 EditBox 읽기 전용
    for (int id : { IDC_EDIT_MI_ID, IDC_EDIT_MI_NAME,
                    IDC_EDIT_MI_PHONE, IDC_EDIT_MI_ADDR, IDC_EDIT_MI_GRADE })
    {
        CWnd* p = GetDlgItem(id);
        if (p) p->EnableWindow(FALSE);
    }

    // 아이디는 AuthManager에서 바로 표시
    std::string uid = AuthManager::GetInstance().GetCurrentUserID();
    SetDlgItemText(IDC_EDIT_MI_ID, CA2T(uid.c_str(), CP_UTF8));

    // 서버 콜백 등록
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(
        CmdCustomer::REQ_MY_INFO,
        [hThis](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            ::PostMessage(hThis, WM_MYINFO_RESPONSE, 0, (LPARAM)p);
        });

    RequestMyInfo();
    return TRUE;
}

void MyInfoDlg::RequestMyInfo()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) {
        // 오프라인: 아이디만 표시, 나머지 "-"
        SetDlgItemText(IDC_EDIT_MI_NAME,  _T("-"));
        SetDlgItemText(IDC_EDIT_MI_PHONE, _T("-"));
        SetDlgItemText(IDC_EDIT_MI_ADDR,  _T("-"));
        SetDlgItemText(IDC_EDIT_MI_GRADE, _T("-"));
        return;
    }
    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json  = "{\"token\":\"" + token + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER,
                   CmdCustomer::REQ_MY_INFO, json);

    SetDlgItemText(IDC_EDIT_MI_NAME,  _T("조회 중..."));
}

LRESULT MyInfoDlg::OnMyInfoResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (MIJInt(*pBody, "status") == (int)Status::SUCCESS)
        FillFields(*pBody);
    else
        SetDlgItemText(IDC_EDIT_MI_NAME, _T("조회 실패"));

    delete pBody;
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MY_INFO);
    return 0;
}

void MyInfoDlg::FillFields(const std::string& body)
{
    auto set = [&](int ctrlID, const std::string& key) {
        std::string val = MIJStr(body, key);
        SetDlgItemText(ctrlID, CA2T(val.empty() ? "-" : val.c_str(), CP_UTF8));
    };
    set(IDC_EDIT_MI_NAME,  "name");
    set(IDC_EDIT_MI_PHONE, "phone");
    set(IDC_EDIT_MI_ADDR,  "address");
    set(IDC_EDIT_MI_GRADE, "grade");
}

void MyInfoDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MY_INFO);
    EndDialog(IDCANCEL);
}
