// ================================================================
//  MenuDetailDlg.cpp  ─  메뉴 상세 / 옵션 선택 / 장바구니 담기
//
//  [변경사항]
//  - 다른 가게 메뉴 담기 시 팝업 메시지 완성
//  - 수량(+/-) 버튼 및 실시간 합계 갱신 추가
//  - 옵션 체크 변경 시 합계 자동 갱신
//
//  [IDC 목록] ← resource.h 에 추가 필요
//    IDC_LIST_OPTIONS_D       기존
//    IDC_STATIC_MENU_NAME_D   기존
//    IDC_STATIC_BASE_PRICE_D  기존
//    IDC_BTN_BACK             기존
//    IDC_BTN_CART             기존
//    IDC_BTN_COUNT_MINUS_D    2010  ← 수량 감소
//    IDC_BTN_COUNT_PLUS_D     2011  ← 수량 증가
//    IDC_EDIT_COUNT_D         2012  ← 수량 표시
//    IDC_STATIC_TOTAL_D       2013  ← 합계 금액 표시
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "MenuDetailDlg.h"
#include "OrderManager.h"
#include "ImageLoader.h"

// ── IDC 임시 정의 (resource.h 에 없으면) ─────────────────────
#ifndef IDC_BTN_COUNT_MINUS_D
#define IDC_BTN_COUNT_MINUS_D   2010
#define IDC_BTN_COUNT_PLUS_D    2011
#define IDC_EDIT_COUNT_D        2012
#define IDC_STATIC_TOTAL_D      2013
#endif

IMPLEMENT_DYNAMIC(MenuDetailDlg, CDialogEx)

MenuDetailDlg::MenuDetailDlg(CWnd* pParent)
    : CDialogEx(IDD_MENUDETAIL_DLG, pParent)
    , m_nQuantity(1)
{}
MenuDetailDlg::~MenuDetailDlg() {}

void MenuDetailDlg::OnDestroy()
{
    if (m_hMenuImg) { ::DeleteObject(m_hMenuImg); m_hMenuImg = nullptr; }
    CDialogEx::OnDestroy();
}

void MenuDetailDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_OPTIONS_D,    m_listOptions);
    DDX_Text(pDX,    IDC_STATIC_MENU_NAME_D, m_strMenuName);
}

BEGIN_MESSAGE_MAP(MenuDetailDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,             &MenuDetailDlg::OnBnClickedBtnBack)
    ON_BN_CLICKED(IDC_BTN_CART,             &MenuDetailDlg::OnBnClickedBtnCart)
    ON_BN_CLICKED(IDC_BTN_COUNT_MINUS_D,    &MenuDetailDlg::OnBnClickedCountMinus)
    ON_BN_CLICKED(IDC_BTN_COUNT_PLUS_D,     &MenuDetailDlg::OnBnClickedCountPlus)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OPTIONS_D, &MenuDetailDlg::OnLvnItemchangedOptions)
    ON_WM_PAINT()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL MenuDetailDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 메뉴명 표시
    SetDlgItemText(IDC_STATIC_MENU_NAME_D, m_strMenuName);

    // 기본 가격 표시
    CString strPrice;
    strPrice.Format(_T("%d원"), m_menuInfo.price);
    SetDlgItemText(IDC_STATIC_BASE_PRICE_D, strPrice);

    // 옵션 리스트 초기화
    m_listOptions.SetExtendedStyle(
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
    m_listOptions.InsertColumn(0, _T("옵션명"), LVCFMT_LEFT,  150);
    m_listOptions.InsertColumn(1, _T("추가금액"), LVCFMT_RIGHT, 80);

    InsertOptionData();

    // 수량 초기값
    m_nQuantity = 1;
    SetDlgItemInt(IDC_EDIT_COUNT_D, m_nQuantity);

    // 합계 초기 표시
    UpdateTotal();

    // 음식 이미지 로드
    LoadMenuImage();

    return TRUE;
}

// ── 서버 이미지 로드 (GDI+ 기반) ────────────────────────────
void MenuDetailDlg::LoadMenuImage()
{
    if (m_menuInfo.menuImageUrl.empty()) return;

    CString relPath = CA2T(m_menuInfo.menuImageUrl.c_str(), CP_UTF8);
    CString fullPath = ImageLoader::MakeServerPath(relPath);
    m_hMenuImg = ImageLoader::LoadResized(fullPath, 120, 120);

    if (m_hMenuImg) Invalidate();
}

// ── 음식 이미지 그리기 (다이얼로그 상단 고정 위치) ───────────
void MenuDetailDlg::OnPaint()
{
    CPaintDC dc(this);
    if (!m_hMenuImg) {
        CDialogEx::OnPaint();
        return;
    }

    // 이미지를 다이얼로그 상단 오른쪽에 120×120으로 표시
    BITMAP bm = {};
    ::GetObject(m_hMenuImg, sizeof(bm), &bm);

    CRect rcClient; GetClientRect(&rcClient);
    int imgW = 120, imgH = 120;
    int x = rcClient.right - imgW - 8;
    int y = 8;

    HDC hdcMem = ::CreateCompatibleDC(dc.m_hDC);
    HGDIOBJ hOld = ::SelectObject(hdcMem, m_hMenuImg);
    ::SetStretchBltMode(dc.m_hDC, HALFTONE);
    ::StretchBlt(dc.m_hDC, x, y, imgW, imgH,
                 hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    ::SelectObject(hdcMem, hOld);
    ::DeleteDC(hdcMem);

    // 나머지 컨트롤 다시 그리기
    CDialogEx::OnPaint();
}

// ── 옵션 리스트 채우기 ────────────────────────────────────────
void MenuDetailDlg::InsertOptionData()
{
    m_listOptions.DeleteAllItems();
    int row = 0;
    for (const auto& og : m_menuInfo.optionGroups) {
        // 그룹명을 구분 행으로 삽입 (체크 불가)
        if (!og.groupName.empty()) {
            CString grpName = CA2T(og.groupName.c_str(), CP_UTF8);
            if (og.isRequired) grpName += _T(" *");  // 필수 표시
            int idx = m_listOptions.InsertItem(row++, grpName);
            m_listOptions.SetItemText(idx, 1, _T(""));
            // 그룹 행은 체크박스 비활성화 (상태로 구분)
            m_listOptions.SetItemState(idx, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
        }
        for (const auto& oi : og.items) {
            CString strName  = CA2T(oi.optionName.c_str(), CP_UTF8);
            CString strPrice;
            strPrice.Format(_T("+%d원"), oi.optionPrice);
            int idx = m_listOptions.InsertItem(row++, strName);
            m_listOptions.SetItemText(idx, 1, strPrice);
        }
    }
    if (row == 0)
        m_listOptions.InsertItem(0, _T("선택 가능한 옵션이 없습니다."));
}

// ── 합계 금액 갱신 ───────────────────────────────────────────
void MenuDetailDlg::UpdateTotal()
{
    int optionTotal = 0;
    int row = 0;
    for (const auto& og : m_menuInfo.optionGroups) {
        if (!og.groupName.empty()) row++; // 그룹 행 건너뜀
        for (const auto& oi : og.items) {
            if (m_listOptions.GetCheck(row))
                optionTotal += oi.optionPrice;
            row++;
        }
    }

    int total = (m_menuInfo.price + optionTotal) * m_nQuantity;
    CString strTotal;
    strTotal.Format(_T("%d원"), total);
    SetDlgItemText(IDC_STATIC_TOTAL_D, strTotal);

    // 담기 버튼에도 금액 표시
    //CString strBtn;
    //strBtn.Format(_T("/*장바구니 담기 (%d원)"), total);
    //CWnd* pBtn = this->GetDlgItem(IDC_BTN_CART);
    //if (pBtn) pBtn->SetWindowText(strBtn);
}

// ── 수량 감소 ─────────────────────────────────────────────────
void MenuDetailDlg::OnBnClickedCountMinus()
{
    if (m_nQuantity > 1) {
        m_nQuantity--;
        SetDlgItemInt(IDC_EDIT_COUNT_D, m_nQuantity);
        UpdateTotal();
    }
}

// ── 수량 증가 ─────────────────────────────────────────────────
void MenuDetailDlg::OnBnClickedCountPlus()
{
    if (m_nQuantity < 99) {
        m_nQuantity++;
        SetDlgItemInt(IDC_EDIT_COUNT_D, m_nQuantity);
        UpdateTotal();
    }
}

// ── 옵션 체크 변경 → 합계 갱신 ──────────────────────────────
void MenuDetailDlg::OnLvnItemchangedOptions(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if (pNMLV->uChanged & LVIF_STATE)
        UpdateTotal();
    *pResult = 0;
}

// ── 장바구니 담기 ─────────────────────────────────────────────
void MenuDetailDlg::OnBnClickedBtnCart()
{
    CartItem newItem;
    newItem.menuID    = m_menuInfo.menuID;
    newItem.menuName  = CT2A(m_strMenuName, CP_UTF8);
    newItem.basePrice = m_menuInfo.price;   // ★ 기본가만 저장, 옵션가 더하지 않음
    newItem.quantity  = m_nQuantity;
    newItem.storeID   = OrderManager::GetInstance().GetCurrentStoreID();
    newItem.optionGroups = m_menuInfo.optionGroups;  // ★ 전체 옵션 그룹 보관

    // 체크된 옵션 수집 (그룹 구분 행 건너뜀)
    int row = 0;
    for (const auto& og : m_menuInfo.optionGroups) {
        if (!og.groupName.empty()) row++; // 그룹 행 건너뜀
        for (const auto& oi : og.items) {
            if (m_listOptions.GetCheck(row)) {
                newItem.selectedOptions.push_back(oi);
                //newItem.basePrice += oi.optionPrice;  // ★ basePrice에 더하지 않음
            }
            row++;
        }
    }
    newItem.CalculateTotalPrice();

    int storeID = OrderManager::GetInstance().GetCurrentStoreID();
    bool added  = OrderManager::GetInstance().AddToCart(storeID, newItem);

    if (!added) {
        // ── 다른 가게 메뉴가 이미 장바구니에 있는 경우 ────────
        int ret = AfxMessageBox(
            _T("장바구니에 다른 가게의 메뉴가 있습니다.\n")
            _T("현재 장바구니를 비우고 이 메뉴를 담으시겠습니까?"),
            MB_YESNO | MB_ICONQUESTION);
        if (ret == IDYES) {
            OrderManager::GetInstance().ClearCart();
            OrderManager::GetInstance().AddToCart(storeID, newItem);
        } else {
            return; // 취소 → 창 유지
        }
    }

    CDialogEx::OnOK();
}

void MenuDetailDlg::OnBnClickedBtnBack() { CDialogEx::OnCancel(); }
void MenuDetailDlg::OnOK()               { OnBnClickedBtnCart(); }
