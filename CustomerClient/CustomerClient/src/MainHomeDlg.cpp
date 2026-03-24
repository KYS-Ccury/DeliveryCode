#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MainHomeDlg.h"
#include "OrderManager.h"
#include "CartDlg.h"
#include "StoreListDlg.h"
#include "OrderHistoryDlg.h"
#include "AuthManager.h"

IMPLEMENT_DYNAMIC(MainHomeDlg, CDialogEx)

MainHomeDlg::MainHomeDlg(CWnd* pParent)
    : CDialogEx(IDD_MAINHOME_DLG, pParent) {}
MainHomeDlg::~MainHomeDlg() {}

void MainHomeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_STOR, m_listStore);
    // ★ DDX_Control(IDC_STATIC_MENU_BAR, m_wndScrollMenu) 제거
    //   RC 의 LTEXT 와 CScrollMenu 타입 불일치 → 크래시 원인
    //   대신 OnInitDialog 에서 수동 Create
}

BEGIN_MESSAGE_MAP(MainHomeDlg, CDialogEx)
    ON_WM_MOUSEWHEEL()
    ON_WM_CTLCOLOR()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED, &MainHomeDlg::OnScrollMenuClicked)
    ON_BN_CLICKED(IDC_BUTTON1,         &MainHomeDlg::OnBnClickedButton1)
    ON_NOTIFY(NM_CLICK, IDC_LIST_STOR, &MainHomeDlg::OnNMDblclkListStor)
    ON_BN_CLICKED(IDC_BTN_MYPAGE,      &MainHomeDlg::OnBnClickedBtnOrderHistory)
END_MESSAGE_MAP()

BOOL MainHomeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_THICKFRAME, WS_CLIPCHILDREN | WS_DLGFRAME);
    CenterWindow();

    // ── 리스트 컨트롤 설정 ──────────────────────────────
    m_listStore.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listStore.InsertColumn(0, _T("매장명"),      LVCFMT_LEFT,   200);
    m_listStore.InsertColumn(1, _T("배달시간"),    LVCFMT_CENTER, 100);
    m_listStore.InsertColumn(2, _T("최소주문/거리"), LVCFMT_RIGHT, 140);
    m_listStore.InsertColumn(3, _T("거리"),        LVCFMT_CENTER,  60);

    // ── CScrollMenu 수동 Create ─────────────────────────
    // RC 의 IDC_STATIC_MENU_BAR(LTEXT) 위치에 CScrollMenu 윈도우를 생성
    CWnd* pPlaceholder = GetDlgItem(IDC_STATIC_MENU_BAR);
    if (pPlaceholder)
    {
        CRect rect;
        pPlaceholder->GetWindowRect(&rect);
        ScreenToClient(&rect);
        pPlaceholder->ShowWindow(SW_HIDE);   // placeholder 숨기기

        m_wndScrollMenu.Create(
            _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_NOTIFY,
            rect, this, IDC_STATIC_MENU_BAR);
    }

    // ── 데이터 로드 ─────────────────────────────────────
    OrderManager::GetInstance().LoadStoreData();

    auto rawCats = OrderManager::GetInstance().GetCategoryList();
    m_vecCategories.clear();
    for (const auto& cat : rawCats)
        m_vecCategories.push_back(CString(CA2T(cat.c_str(), CP_UTF8)));

    if (!m_vecCategories.empty())
        UpdateStoreListUI(m_vecCategories[0]);

    // Create 성공한 경우에만 SetMenuItems 호출
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecCategories);

    m_brushBack.CreateSolidBrush(RGB(230, 245, 245));
    m_brushWhite.CreateSolidBrush(RGB(255, 255, 255));

    return TRUE;
}

void MainHomeDlg::UpdateStoreListUI(CString categoryName)
{
    m_listStore.DeleteAllItems();
    m_vecStoreCache.clear();

    std::string targetCat = std::string(CT2A(categoryName, CP_UTF8));
    auto stores = OrderManager::GetInstance().GetStoresByCategory(targetCat);

    for (int i = 0; i < (int)stores.size(); ++i) {
        CString strName = CA2T(stores[i].storeName.c_str(), CP_UTF8);
        CString strTime = CA2T(stores[i].deliveryTime.c_str(), CP_UTF8);
        int nRow = m_listStore.InsertItem(i, strName);
        m_listStore.SetItemText(nRow, 1, strTime);
        CString strInfo;
        strInfo.Format(_T("%d원"), stores[i].minOrderAmount);
        m_listStore.SetItemText(nRow, 2, strInfo);
        CString strDist;
        strDist.Format(_T("%.1fkm"), stores[i].distance);
        m_listStore.SetItemText(nRow, 3, strDist);
        m_vecStoreCache.push_back(stores[i]);
    }
}

LRESULT MainHomeDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM lParam)
{
    int nIndex = (UINT)wParam - 2000;
    if (nIndex >= 0 && nIndex < (int)m_vecCategories.size())
        UpdateStoreListUI(m_vecCategories[nIndex]);
    return 0;
}

BOOL MainHomeDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (m_wndScrollMenu.GetSafeHwnd()) {
        CRect rect;
        m_wndScrollMenu.GetWindowRect(&rect);
        if (rect.PtInRect(pt))
            return m_wndScrollMenu.OnMouseWheel(nFlags, zDelta, pt);
    }
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

HBRUSH MainHomeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    int nID = pWnd->GetDlgCtrlID();
    if (nID == IDC_STATIC_TOP_BG || nID == IDC_STATIC_MENU_BAR) {
        pDC->SetBkColor(RGB(230, 245, 245));
        if (m_brushBack.GetSafeHandle())
            return (HBRUSH)m_brushBack.GetSafeHandle();
    }
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        pDC->SetBkMode(TRANSPARENT);
        if (m_brushWhite.GetSafeHandle())
            return (HBRUSH)m_brushWhite.GetSafeHandle();
    }
    return hbr;
}

void MainHomeDlg::OnNMDblclkListStor(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int nIndex = pNMIA->iItem;
    if (nIndex >= 0 && nIndex < (int)m_vecStoreCache.size()) {
        OrderManager::GetInstance().SelectStore(m_vecStoreCache[nIndex].storeID);
        StoreListDlg dlg;
        dlg.m_strStoreName = CA2T(m_vecStoreCache[nIndex].storeName.c_str(), CP_UTF8);
        dlg.m_storeInfo    = m_vecStoreCache[nIndex];
        dlg.DoModal();
    }
    *pResult = 0;
}

void MainHomeDlg::OnBnClickedButton1()
{
    CartDlg dlg;
    dlg.DoModal();
}

void MainHomeDlg::OnBnClickedBtnOrderHistory()
{
    OrderHistoryDlg dlg;
    dlg.DoModal();
}
