#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "StoreListDlg.h"
#include "OrderManager.h"
#include "CartDlg.h"
#include "StoreDetailDlg.h"
#include "MenuDetailDlg.h"

IMPLEMENT_DYNAMIC(StoreListDlg, CDialogEx)

StoreListDlg::StoreListDlg(CWnd* pParent)
    : CDialogEx(IDD_STORELIST_DLG, pParent) {}
StoreListDlg::~StoreListDlg() {}

void StoreListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_MENU_ITEMS,     m_listMenu);
    DDX_Control(pDX, IDC_STATIC_SUB_MENU_BAR, m_wndScrollMenu);
}

BEGIN_MESSAGE_MAP(StoreListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,        &StoreListDlg::OnBnClickedBtnBack)
    ON_WM_MOUSEWHEEL()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED, &StoreListDlg::OnScrollMenuClicked)
    ON_BN_CLICKED(IDC_BTN_CART,        &StoreListDlg::OnBnClickedBtnCart)
    ON_BN_CLICKED(IDC_BTN_STORE_INFO,  &StoreListDlg::OnBnClickedBtnStoreInfo)
    ON_NOTIFY(NM_CLICK, IDC_LIST_MENU_ITEMS, &StoreListDlg::OnNMClickListMenuItems)
END_MESSAGE_MAP()

BOOL StoreListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_CAPTION, 0);
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN);
    CenterWindow();

    m_listMenu.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMenu.InsertColumn(0, _T("메뉴명"),  LVCFMT_LEFT,  150);
    m_listMenu.InsertColumn(1, _T("설명"),    LVCFMT_LEFT,  200);
    m_listMenu.InsertColumn(2, _T("가격"),    LVCFMT_RIGHT,  80);

    if (!m_strStoreName.IsEmpty()) SetWindowText(m_strStoreName);

    m_vecSubCategories = { _T("전체"), _T("인기메뉴"), _T("세트메뉴"), _T("단품"), _T("음료") };
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecSubCategories);

    UpdateMenuListUI(m_vecSubCategories[0]);
    return TRUE;
}

void StoreListDlg::UpdateMenuListUI(CString subCategory)
{
    m_listMenu.DeleteAllItems();
    m_vecMenuCache.clear();

    // ── 수정 포인트 ──────────────────────────────────────
    // 1. storeID 는 m_storeInfo 에서 한 번만 가져옴 (중복 선언 제거)
    // 2. GetMenuData() 로 menus 변수 정상 획득
    int storeID = m_storeInfo.storeID;
    std::string sub = CT2A(subCategory, CP_UTF8);
    std::vector<MenuInfo> menus = OrderManager::GetInstance().GetMenuData(storeID, sub);
    // ────────────────────────────────────────────────────

    if (menus.empty()) {
        m_listMenu.InsertItem(0, _T("메뉴 정보가 없습니다."));
        return;
    }
    for (int i = 0; i < (int)menus.size(); ++i) {
        // CA2T 결과를 변수에 먼저 저장 (Format 인자로 직접 전달 금지)
        CString strName = CA2T(menus[i].menuName.c_str(), CP_UTF8);
        CString strPrice;
        strPrice.Format(_T("%d원"), menus[i].price);
        int nRow = m_listMenu.InsertItem(i, strName);
        m_listMenu.SetItemText(nRow, 1, _T(""));
        m_listMenu.SetItemText(nRow, 2, strPrice);
        m_vecMenuCache.push_back(menus[i]);
    }
}

LRESULT StoreListDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM lParam)
{
    int nIndex = (UINT)wParam - 2000;
    if (nIndex >= 0 && nIndex < (int)m_vecSubCategories.size())
        UpdateMenuListUI(m_vecSubCategories[nIndex]);
    return 0;
}

BOOL StoreListDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    CRect rect;
    m_wndScrollMenu.GetWindowRect(&rect);
    if (rect.PtInRect(pt)) return m_wndScrollMenu.OnMouseWheel(nFlags, zDelta, pt);
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void StoreListDlg::OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int nIndex = pNMIA->iItem;
    if (nIndex >= 0 && nIndex < (int)m_vecMenuCache.size()) {
        MenuDetailDlg dlg;
        dlg.m_strMenuName = CA2T(m_vecMenuCache[nIndex].menuName.c_str(), CP_UTF8);
        dlg.m_menuInfo    = m_vecMenuCache[nIndex];
        dlg.DoModal();
    }
    *pResult = 0;
}

void StoreListDlg::OnBnClickedBtnCart()
{
    CartDlg dlg;
    if (dlg.DoModal() == IDOK) CDialogEx::OnOK();
}

void StoreListDlg::OnBnClickedBtnStoreInfo()
{
    StoreDetailDlg dlg;
    dlg.m_strName = CA2T(m_storeInfo.storeName.c_str(),   CP_UTF8);
    dlg.m_strAddr = CA2T(m_storeInfo.address.c_str(),     CP_UTF8);
    dlg.m_strTime = CA2T(m_storeInfo.openTime.c_str(),    CP_UTF8);
    dlg.m_strOff  = CA2T(m_storeInfo.holiday.c_str(),     CP_UTF8);
    dlg.m_strTel  = CA2T(m_storeInfo.phoneNumber.c_str(), CP_UTF8);
    dlg.DoModal();
}

void StoreListDlg::OnBnClickedBtnBack() { CDialogEx::OnCancel(); }
