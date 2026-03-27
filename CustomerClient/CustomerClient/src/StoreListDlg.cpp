// ================================================================
//  StoreListDlg.cpp  ─  가게 상세 / 메뉴 목록 (리뷰 버튼 추가)
//
//  [추가]
//  IDC_BTN_STORE_INFO 클릭 → 팝업 메뉴
//    1. 가게 정보 → StoreDetailDlg
//    2. 리뷰 보기 → ReviewListDlg (m_bCanWriteReview 자동 판단)
// ================================================================
#include "pch.h"
#include "CustomerClient.h"
#include "afxdialogex.h"
#include "StoreListDlg.h"
#include "OrderManager.h"
#include "CartDlg.h"
#include "StoreDetailDlg.h"
#include "MenuDetailDlg.h"
#include "ReviewListDlg.h"
#include "NetworkManager.h"
#include "ImageLoader.h"
#include "AuthManager.h"
#include "common/header/Types.h"

#define WM_MENU_LIST_RESPONSE (WM_USER + 120)

// 메뉴 썸네일 크기
static const int MENU_THUMB_W = 70;
static const int MENU_THUMB_H = 70;
static const TCHAR* MENU_IMAGE_ROOT = _T("\\\\10.10.10.122\\images\\");

// ── JSON 헬퍼 ────────────────────────────────────────────────
static std::string SLJStr(const std::string& j, const std::string& k)
{
    std::string t="\""+k+"\":\""; auto p=j.find(t);
    if(p==std::string::npos) return "";
    p+=t.size(); auto e=j.find('"',p);
    return (e==std::string::npos)?"":j.substr(p,e-p);
}
static int SLJInt(const std::string& j, const std::string& k)
{
    std::string t="\""+k+"\":"; auto p=j.find(t);
    if(p==std::string::npos) return 0;
    try{return std::stoi(j.substr(p+t.size()));}catch(...){return 0;}
}
static bool SLJBool(const std::string& j, const std::string& k)
{
    std::string t="\""+k+"\":"; auto p=j.find(t);
    if(p==std::string::npos) return false;
    return j.substr(p+t.size(),4)=="true";
}
static std::vector<std::string> SLExtractObjs(const std::string& j, const std::string& arrKey)
{
    std::vector<std::string> res;
    std::string t="\""+arrKey+"\":["; auto ap=j.find(t);
    if(ap==std::string::npos) return res;
    size_t i=ap+t.size();
    while(i<j.size()){
        auto s=j.find('{',i); if(s==std::string::npos) break;
        int d=0; size_t e=s;
        for(;e<j.size();++e){ if(j[e]=='{')++d; else if(j[e]=='}'){ if(--d==0) break; } }
        res.push_back(j.substr(s,e-s+1)); i=e+1;
    }
    return res;
}
static MenuInfo ParseMenuObj(const std::string& obj)
{
    MenuInfo m;
    // 서버가 "id" 키로 전송 (menu_id 아님)
    m.menuID      = SLJInt(obj,"id");
    m.menuName    = SLJStr(obj,"name");
    m.description = SLJStr(obj,"desc");    // ★ 메뉴 설명 파싱 추가
    m.price       = SLJInt(obj,"price");
    m.subCategory = SLJStr(obj,"sub_category");
    m.menuImageUrl= SLJStr(obj,"image_url");
    // 서버가 "option_groups":[{group_name, options:[{option_id,name,price}]}] 형태로 전송
    auto grps = SLExtractObjs(obj,"option_groups");
    for (const auto& g : grps) {
        OptionGroup og;
        og.groupName  = SLJStr(g,"group_name");
        og.isRequired = SLJBool(g,"is_required");
        auto items = SLExtractObjs(g,"options");   // "options" 키 (서버 응답 기준)
        for (const auto& it : items) {
            OptionItem oi;
            oi.optionID    = SLJInt(it,"option_id");
            oi.optionName  = SLJStr(it,"name");
            oi.optionPrice = SLJInt(it,"price");
            og.items.push_back(oi);
        }
        m.optionGroups.push_back(og);
    }
    return m;
}

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
    ON_BN_CLICKED(IDC_BTN_BACK,              &StoreListDlg::OnBnClickedBtnBack)
    ON_WM_MOUSEWHEEL()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED,       &StoreListDlg::OnScrollMenuClicked)
    ON_MESSAGE(WM_MENU_LIST_RESPONSE,        &StoreListDlg::OnMenuListResponse)
    ON_BN_CLICKED(IDC_BTN_CART,              &StoreListDlg::OnBnClickedBtnCart)
    ON_BN_CLICKED(IDC_BTN_STORE_INFO,        &StoreListDlg::OnBnClickedBtnStoreInfo)
    ON_NOTIFY(NM_CLICK, IDC_LIST_MENU_ITEMS, &StoreListDlg::OnNMClickListMenuItems)
END_MESSAGE_MAP()

BOOL StoreListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_CAPTION, 0);
    CenterWindow();

    // 메뉴 썸네일 ImageList 초기화
    m_imgListMenu.Create(MENU_THUMB_W, MENU_THUMB_H, ILC_COLOR32, 16, 8);
    m_listMenu.SetImageList(&m_imgListMenu, LVSIL_SMALL);

    m_listMenu.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMenu.InsertColumn(0, _T("사진"),   LVCFMT_LEFT,    76);  // 이미지 컬럼
    m_listMenu.InsertColumn(1, _T("메뉴명"), LVCFMT_LEFT,   160);
    m_listMenu.InsertColumn(2, _T("설명"),   LVCFMT_LEFT,   150);
    m_listMenu.InsertColumn(3, _T("가격"),   LVCFMT_RIGHT,   80);

    if (!m_strStoreName.IsEmpty()) SetWindowText(m_strStoreName);

    m_vecSubCategories = { _T("전체"),_T("인기메뉴"),_T("세트메뉴"),_T("단품"),_T("음료") };
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecSubCategories);

    RegisterMenuCallback();
    SendMenuListRequest(_T("전체"));
    return TRUE;
}

void StoreListDlg::RegisterMenuCallback()
{
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_MENU_LIST,
        [this](uint16_t, const std::string& body) {
            std::string* p = new std::string(body);
            PostMessage(WM_MENU_LIST_RESPONSE, 0, (LPARAM)p);
        });
}
void StoreListDlg::SendMenuListRequest(const CString& subCat)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) { UpdateMenuListUI(subCat); return; }
    std::string cat = (subCat==_T("전체")) ? "" : std::string(CT2A(subCat,CP_UTF8));
    // 서버가 "store_id"와 "sub_category" 키를 사용
    std::string json = "{\"store_id\":" + std::to_string(m_storeInfo.storeID)
                     + ",\"sub_category\":\"" + cat + "\"}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_MENU_LIST, json);
    m_listMenu.DeleteAllItems();
    m_listMenu.InsertItem(0, _T("메뉴를 불러오는 중..."));
}
LRESULT StoreListDlg::OnMenuListResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;
    if (SLJInt(*pBody,"status") == (int)Status::SUCCESS) {
        m_vecMenuCache.clear();
        for (const auto& obj : SLExtractObjs(*pBody,"menus")) {
            MenuInfo m = ParseMenuObj(obj);
            if (m.menuID > 0) m_vecMenuCache.push_back(m);
        }
        // 서브카테고리 동적 갱신
        std::vector<CString> cats = { _T("전체") };
        for (const auto& m : m_vecMenuCache) {
            CString c = CA2T(m.subCategory.c_str(),CP_UTF8);
            bool found = false;
            for (const auto& x : cats) if(x==c){found=true;break;}
            if (!found && !c.IsEmpty()) cats.push_back(c);
        }
        m_vecSubCategories = cats;
        if (m_wndScrollMenu.GetSafeHwnd())
            m_wndScrollMenu.SetMenuItems(m_vecSubCategories);
        RebuildMenuListUI(m_vecMenuCache, _T("전체"));
    } else {
        m_listMenu.DeleteAllItems();
        m_listMenu.InsertItem(0, _T("메뉴 정보를 가져오지 못했습니다."));
    }
    delete pBody; return 0;
}
// ── 서버 이미지 로드 헬퍼 (GDI+ 사용) ──────────────────────
HBITMAP StoreListDlg::LoadMenuImage(const CString& relPath)
{
    CString fullPath = ImageLoader::MakeServerPath(relPath);
    return ImageLoader::Load(fullPath);
}
void StoreListDlg::ResizeBitmapTo(HBITMAP& hBmp, int w, int h)
{
    // GDI+의 LoadResized로 대체되어 직접 호출되지 않음 (호환성 유지)
    if (!hBmp) return;
}

void StoreListDlg::RebuildMenuListUI(const std::vector<MenuInfo>& menus, const CString& filter)
{
    m_listMenu.DeleteAllItems();

    // ImageList 재구성
    if (m_imgListMenu.GetSafeHandle()) m_imgListMenu.DeleteImageList();
    m_imgListMenu.Create(MENU_THUMB_W, MENU_THUMB_H, ILC_COLOR32,
                         (int)menus.size() + 1, 4);
    m_listMenu.SetImageList(&m_imgListMenu, LVSIL_SMALL);

    // 기본 이미지 (회색 박스) — 화면DC 기반 32bpp
    HBITMAP hDef = nullptr;
    {
        HDC hdcScreen = ::GetDC(nullptr);
        HDC hdc = ::CreateCompatibleDC(hdcScreen);
        hDef = ::CreateCompatibleBitmap(hdcScreen, MENU_THUMB_W, MENU_THUMB_H);
        HGDIOBJ hOld = ::SelectObject(hdc, hDef);
        RECT rc = {0, 0, MENU_THUMB_W, MENU_THUMB_H};
        HBRUSH hBr = ::CreateSolidBrush(RGB(210, 210, 210));
        ::FillRect(hdc, &rc, hBr); ::DeleteObject(hBr);
        ::SelectObject(hdc, hOld); ::DeleteDC(hdc);
        ::ReleaseDC(nullptr, hdcScreen);
    }
    int defIdx = m_imgListMenu.Add(CBitmap::FromHandle(hDef), (CBitmap*)nullptr);
    ::DeleteObject(hDef);

    int row = 0;
    for (const auto& m : menus) {
        CString sub = CA2T(m.subCategory.c_str(),CP_UTF8);
        if (filter != _T("전체") && sub != filter) continue;

        // 이미지 로드 (GDI+ 기반)
        int imgIdx = defIdx;
        CString imgUrl = CA2T(m.menuImageUrl.c_str(), CP_UTF8);
        if (!imgUrl.IsEmpty()) {
            CString fullPath = ImageLoader::MakeServerPath(imgUrl);
            HBITMAP hBmp = ImageLoader::LoadResized(fullPath, MENU_THUMB_W, MENU_THUMB_H);
            if (hBmp) {
                imgIdx = m_imgListMenu.Add(CBitmap::FromHandle(hBmp), (CBitmap*)nullptr);
                ::DeleteObject(hBmp);
            }
        }

        CString n = CA2T(m.menuName.c_str(),CP_UTF8);
        CString d = CA2T(m.description.c_str(),CP_UTF8);
        CString p; p.Format(_T("%d원"), m.price);

        // 컬럼 0: 이미지
        LVITEM lvi2 = {};
        lvi2.mask     = LVIF_TEXT | LVIF_IMAGE;
        lvi2.iItem    = row++;
        lvi2.iSubItem = 0;
        lvi2.pszText  = (LPTSTR)(LPCTSTR)_T("");
        lvi2.iImage   = imgIdx;
        int r = m_listMenu.InsertItem(&lvi2);
        m_listMenu.SetItemText(r, 1, n);  // 메뉴명
        m_listMenu.SetItemText(r, 2, d);  // 설명
        m_listMenu.SetItemText(r, 3, p);  // 가격
    }
    if (row == 0) m_listMenu.InsertItem(0, _T("메뉴가 없습니다."));
}
void StoreListDlg::UpdateMenuListUI(CString subCat)
{
    m_listMenu.DeleteAllItems(); m_vecMenuCache.clear();
    std::string subCatStr = CT2A(subCat, CP_UTF8);
    m_vecMenuCache = OrderManager::GetInstance().GetMenuData(m_storeInfo.storeID, subCatStr);
    RebuildMenuListUI(m_vecMenuCache, subCat);
}
LRESULT StoreListDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM)
{
    int n=(UINT)wParam- SCROLL_MENU_BTN_ID;
    if(n<0||n>=(int)m_vecSubCategories.size()) return 0;
    CString sel=m_vecSubCategories[n];
    if(m_vecMenuCache.empty()) SendMenuListRequest(sel);
    else RebuildMenuListUI(m_vecMenuCache,sel);
    return 0;
}
void StoreListDlg::OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult)
{
    int n = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR)->iItem;
    if (n < 0) { *pResult=0; return; }
    CString sel = m_listMenu.GetItemText(n,0);
    // 컬럼 1이 메뉴명 (컬럼 0은 이미지)
    CString sel1 = m_listMenu.GetItemText(n, 1);
    if (!sel.IsEmpty() && sel1.IsEmpty()) sel1 = sel;
    else if (!sel1.IsEmpty()) sel = sel1;
    for (auto& m : m_vecMenuCache) {
        if (CA2T(m.menuName.c_str(),CP_UTF8) == sel ||
            CA2T(m.menuName.c_str(),CP_UTF8) == sel1) {
            MenuDetailDlg dlg(this);
            dlg.m_strMenuName = sel;
            dlg.m_menuInfo    = m;
            dlg.m_menuInfo.storeID = m_storeInfo.storeID;
            dlg.DoModal(); break;
        }
    }
    *pResult=0;
}
void StoreListDlg::OnBnClickedBtnCart()
{
    CartDlg dlg(this); if(dlg.DoModal()==IDOK) CDialogEx::OnOK();
}

// ── 가게정보 버튼 → 팝업 메뉴 (가게정보 / 리뷰 보기) ──────
void StoreListDlg::OnBnClickedBtnStoreInfo()
{
    CMenu menu; menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1, _T("가게 정보"));
    menu.AppendMenu(MF_STRING, 2, _T("리뷰 보기"));

    CRect rect; this->GetDlgItem(IDC_BTN_STORE_INFO)->GetWindowRect(&rect);
    int nCmd = (int)menu.TrackPopupMenu(
        TPM_RETURNCMD | TPM_LEFTALIGN | TPM_BOTTOMALIGN,
        rect.left, rect.top, this);

    if (nCmd == 1) {
        // 가게 정보
        StoreDetailDlg dlg(this);
        dlg.m_strName = CA2T(m_storeInfo.storeName.c_str(), CP_UTF8);
        dlg.m_strAddr = CA2T(m_storeInfo.address.c_str(),   CP_UTF8);
        dlg.m_strTime = CA2T(m_storeInfo.openTime.c_str(),  CP_UTF8);
        dlg.m_strOff  = CA2T(m_storeInfo.holiday.c_str(),   CP_UTF8);
        dlg.m_strTel  = CA2T(m_storeInfo.phoneNumber.c_str(),CP_UTF8);
        dlg.DoModal();
    } else if (nCmd == 2) {
        // 리뷰 목록
        ReviewListDlg dlg(this);
        dlg.m_nStoreID      = m_storeInfo.storeID;
        dlg.m_strStoreName  = CA2T(m_storeInfo.storeName.c_str(), CP_UTF8);
        // 이 가게에서 주문한 적 있으면 리뷰 작성 가능
        // (간단히 OrderManager 주문 내역 확인 또는 서버에서 판단)
        dlg.m_bCanWriteReview = true; // TODO: 실제 구매이력 확인
        dlg.DoModal();
    }
}

void StoreListDlg::OnBnClickedBtnBack()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_MENU_LIST);
    CDialogEx::OnCancel();
}
BOOL StoreListDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    CRect r; m_wndScrollMenu.GetWindowRect(&r);
    if(r.PtInRect(pt)) return m_wndScrollMenu.OnMouseWheel(nFlags,zDelta,pt);
    return CDialogEx::OnMouseWheel(nFlags,zDelta,pt);
}
