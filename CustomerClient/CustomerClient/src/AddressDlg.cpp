// ================================================================
//  AddressDlg.cpp  — 배달 주소 관리 다이얼로그
//
//  IDD_ADDRESS_DLG (2400) 는 CustomerClient.rc 에 추가해야 함
//  (아래 RC 스니펫 참고)
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "AddressDlg.h"
#include "AddressManager.h"

IMPLEMENT_DYNAMIC(AddressDlg, CDialogEx)

BEGIN_MESSAGE_MAP(AddressDlg, CDialogEx)
    ON_BN_CLICKED(IDC_ADDR_ADD,     &AddressDlg::OnBtnAdd)
    ON_BN_CLICKED(IDC_ADDR_DELETE,  &AddressDlg::OnBtnDelete)
    ON_BN_CLICKED(IDC_ADDR_DEFAULT, &AddressDlg::OnBtnSetDefault)
    ON_BN_CLICKED(IDC_ADDR_SELECT,  &AddressDlg::OnBtnSelect)
    ON_BN_CLICKED(IDCANCEL,         &AddressDlg::OnCancel)
END_MESSAGE_MAP()

AddressDlg::AddressDlg(CWnd* pParent)
    : CDialogEx(IDD_ADDRESS_DLG, pParent) {}
AddressDlg::~AddressDlg() {}

void AddressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ADDR_LIST,    m_listAddr);
    DDX_Control(pDX, IDC_ADDR_EDIT,    m_editNewAddr);
}

BOOL AddressDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ::SendMessage(m_editNewAddr.GetSafeHwnd(), EM_SETCUEBANNER, TRUE,
                  (LPARAM)_T("새 주소를 입력하세요 (예: 광주시 북구 용봉동)"));
    RefreshList();
    return TRUE;
}

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

void AddressDlg::OnBtnAdd()
{
    CString s; m_editNewAddr.GetWindowText(s); s.Trim();
    if (s.IsEmpty()) {
        MessageBox(_T("주소를 입력하세요."), _T("알림"), MB_OK); return;
    }
    AddressManager::GetInstance().AddAddress(std::string(CT2A(s, CP_UTF8)));
    m_editNewAddr.SetWindowText(_T(""));
    RefreshList();
    m_listAddr.SetCurSel(AddressManager::GetInstance().Count() - 1);
}

void AddressDlg::OnBtnDelete()
{
    int idx = GetSelectedIndex();
    if (idx < 0) { MessageBox(_T("삭제할 주소를 선택하세요."), _T("알림"), MB_OK); return; }
    if (MessageBox(_T("선택한 주소를 삭제하시겠습니까?"), _T("확인"), MB_YESNO) != IDYES) return;
    AddressManager::GetInstance().DeleteAddress(idx);
    RefreshList();
}

void AddressDlg::OnBtnSetDefault()
{
    int idx = GetSelectedIndex();
    if (idx < 0) { MessageBox(_T("기본으로 설정할 주소를 선택하세요."), _T("알림"), MB_OK); return; }
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
    AddressManager::GetInstance().SetDefault(idx);
    EndDialog(IDOK);
}

void AddressDlg::OnOK()     { OnBtnSelect(); }
void AddressDlg::OnCancel() { EndDialog(IDCANCEL); }
