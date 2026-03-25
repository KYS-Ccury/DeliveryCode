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
#include "AuthManager.h"
#include "common/header/Types.h"

#define WM_MENU_LIST_RESPONSE (WM_USER + 120)

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
    m.menuID      = SLJInt(obj,"menu_id");
    m.menuName    = SLJStr(obj,"name");
    m.price       = SLJInt(obj,"price");
    m.subCategory = SLJStr(obj,"sub_category");
    m.menuImageUrl= SLJStr(obj,"image_url");
    auto grps = SLExtractObjs(obj,"options");
    for (const auto& g : grps) {
        OptionGroup og;
        og.groupName  = SLJStr(g,"group_name");
        og.isRequired = SLJBool(g,"required");
        auto items = SLExtractObjs(g,"items");
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

    m_listMenu.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMenu.InsertColumn(0, _T("메뉴명"), LVCFMT_LEFT,  180);
    m_listMenu.InsertColumn(1, _T("설명"),   LVCFMT_LEFT,  180);
    m_listMenu.InsertColumn(2, _T("가격"),   LVCFMT_RIGHT,  80);

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
    std::string json = "{\"store_id\":"+std::to_string(m_storeInfo.storeID)+",\"category\":\""+cat+"\"}";
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
void StoreListDlg::RebuildMenuListUI(const std::vector<MenuInfo>& menus, const CString& filter)
{
    m_listMenu.DeleteAllItems();
    int row = 0;
    for (const auto& m : menus) {
        CString sub = CA2T(m.subCategory.c_str(),CP_UTF8);
        if (filter != _T("전체") && sub != filter) continue;
        CString n = CA2T(m.menuName.c_str(),CP_UTF8);
        CString p; p.Format(_T("%d원"), m.price);
        int r = m_listMenu.InsertItem(row++, n);
        m_listMenu.SetItemText(r,1,_T(""));
        m_listMenu.SetItemText(r,2,p);
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
    int n=(UINT)wParam-5000;
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
    for (auto& m : m_vecMenuCache) {
        if (CA2T(m.menuName.c_str(),CP_UTF8) == sel) {
            MenuDetailDlg dlg(this);
            dlg.m_strMenuName = sel;
            dlg.m_menuInfo    = m;
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
