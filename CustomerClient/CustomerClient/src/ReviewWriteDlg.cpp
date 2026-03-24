#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "ReviewWriteDlg.h"
#include "AuthManager.h"
#include "NetworkManager.h"
#include "common/json.hpp"

using json = nlohmann::json;

IMPLEMENT_DYNAMIC(ReviewWriteDlg, CDialogEx)

ReviewWriteDlg::ReviewWriteDlg(CWnd* pParent)
    : CDialogEx(IDD_REVIEW_WRITE_DLG, pParent)
    , m_nStarRating(0)
    , m_strReviewText(_T("")) {}
ReviewWriteDlg::~ReviewWriteDlg() {}

void ReviewWriteDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ReviewWriteDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REVIEW_BACK, &ReviewWriteDlg::OnBnClickedBtnReviewBack)
    ON_BN_CLICKED(IDOK,                &ReviewWriteDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL ReviewWriteDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_STAR_RATING);
    pCombo->AddString(_T("5"));
    pCombo->AddString(_T("4"));
    pCombo->AddString(_T("3"));
    pCombo->AddString(_T("2"));
    pCombo->AddString(_T("1"));
    pCombo->SetCurSel(0);
    return TRUE;
}

void ReviewWriteDlg::OnBnClickedBtnReviewBack() { EndDialog(IDCANCEL); }

void ReviewWriteDlg::OnBnClickedOk()
{
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_STAR_RATING);
    m_nStarRating = 5 - pCombo->GetCurSel();
    GetDlgItemText(IDC_EDIT_REVIEW_CONTENT, m_strReviewText);
    if (m_strReviewText.IsEmpty()) { AfxMessageBox(_T("")); return; }

    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    bool result = false;

    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_WRITE_REVIEW,
        [&](uint16_t, const std::string& body) {
            try {
                auto res = json::parse(body);
                result = (res["status"].get<int>() == Status::SUCCESS);
            } catch (...) {}
            SetEvent(hEvent);
        }
    );

    json req;
    req["token"]   = AuthManager::GetInstance().GetAccessToken();
    req["rating"]  = m_nStarRating;
    req["content"] = CT2A(m_strReviewText, CP_UTF8);
    NetworkManager::GetInstance().SendPacket(
        static_cast<uint8_t>(ClientType::CUSTOMER),
        CmdCustomer::REQ_WRITE_REVIEW, req.dump());

    WaitForSingleObject(hEvent, 5000);
    CloseHandle(hEvent);
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_WRITE_REVIEW);

    if (result) CDialogEx::OnOK();
    else AfxMessageBox(_T(""));
}
