// ================================================================
//  StoreListDlg.cpp
//
//  [수정]
//  1. 가게 로고: OnInitDialog → RequestLogoImage() → TCP 217
//     OnLogoResponse → IDC_STATIC_STORE_IMG 컨트롤에 표시
//  2. 메뉴 이미지: RebuildMenuListUI → 기본 회색으로 먼저 표시
//     → RequestMenuImages() → TCP 217
//     → OnMenuImgResponse → ImageList 슬롯 교체
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
#define WM_LOGO_RESPONSE      (WM_USER + 130)
#define WM_MENU_IMG_RESPONSE  (WM_USER + 131)

static const int MENU_THUMB_W = 70;
static const int MENU_THUMB_H = 70;

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
    m.menuID      = SLJInt(obj,"id");
    m.menuName    = SLJStr(obj,"name");
    m.description = SLJStr(obj,"desc");
    m.price       = SLJInt(obj,"price");
    m.subCategory = SLJStr(obj,"sub_category");
    m.menuImageUrl= SLJStr(obj,"image_url");
    auto grps = SLExtractObjs(obj,"option_groups");
    for (const auto& g : grps) {
        OptionGroup og;
        og.groupName  = SLJStr(g,"group_name");
        og.isRequired = SLJBool(g,"is_required");
        auto items = SLExtractObjs(g,"options");
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
StoreListDlg::~StoreListDlg()
{
    NetworkManager::GetInstance().UnregisterCallback(CmdCustomer::REQ_GET_IMAGE);
}

void StoreListDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_MENU_ITEMS,     m_listMenu);
    DDX_Control(pDX, IDC_STATIC_SUB_MENU_BAR, m_wndScrollMenu);
    DDX_Control(pDX, IDC_STATIC_STORE_IMG,    m_storeLogoCtrl);
}

BEGIN_MESSAGE_MAP(StoreListDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_BACK,              &StoreListDlg::OnBnClickedBtnBack)
    ON_WM_MOUSEWHEEL()
    ON_MESSAGE(WM_SCROLL_MENU_CLICKED,       &StoreListDlg::OnScrollMenuClicked)
    ON_MESSAGE(WM_MENU_LIST_RESPONSE,        &StoreListDlg::OnMenuListResponse)
    ON_BN_CLICKED(IDC_BTN_CART,              &StoreListDlg::OnBnClickedBtnCart)
    ON_BN_CLICKED(IDC_BTN_STORE_INFO,        &StoreListDlg::OnBnClickedBtnStoreInfo)
    ON_NOTIFY(NM_CLICK, IDC_LIST_MENU_ITEMS, &StoreListDlg::OnNMClickListMenuItems)
    // ★ 이미지 응답
    ON_MESSAGE(WM_LOGO_RESPONSE,             &StoreListDlg::OnLogoResponse)
    ON_MESSAGE(WM_MENU_IMG_RESPONSE,         &StoreListDlg::OnMenuImgResponse)
END_MESSAGE_MAP()

BOOL StoreListDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ModifyStyle(WS_CAPTION, 0);
    CenterWindow();

    m_imgListMenu.Create(MENU_THUMB_W, MENU_THUMB_H, ILC_COLOR32, 16, 8);
    m_listMenu.SetImageList(&m_imgListMenu, LVSIL_SMALL);

    m_listMenu.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listMenu.InsertColumn(0, _T("사진"),   LVCFMT_LEFT,    76);
    m_listMenu.InsertColumn(1, _T("메뉴명"), LVCFMT_LEFT,   160);
    m_listMenu.InsertColumn(2, _T("설명"),   LVCFMT_LEFT,   150);
    m_listMenu.InsertColumn(3, _T("가격"),   LVCFMT_RIGHT,   80);

    if (!m_strStoreName.IsEmpty()) SetWindowText(m_strStoreName);

    m_vecSubCategories = { _T("전체"),_T("인기메뉴"),_T("세트메뉴"),_T("단품"),_T("음료") };
    if (m_wndScrollMenu.GetSafeHwnd())
        m_wndScrollMenu.SetMenuItems(m_vecSubCategories);

    // ★ REQ_GET_IMAGE 콜백 등록 (로고 + 메뉴 이미지 공용)
    HWND hThis = GetSafeHwnd();
    NetworkManager::GetInstance().RegisterCallback(CmdCustomer::REQ_GET_IMAGE,
        [hThis](uint16_t, const std::string& body) {
            // image_url에 "logo_" 접두사가 있으면 로고, 없으면 메뉴 이미지
            std::string t = "\"image_url\":\"";
            auto p = body.find(t);
            std::string url;
            if (p != std::string::npos) {
                p += t.size();
                auto e = body.find('"', p);
                if (e != std::string::npos) url = body.substr(p, e - p);
            }
            // 로고는 placeholder_ 또는 logo_로 시작하거나 logoUrl에서 온 것
            // 구분: image_url 앞에 "is_logo":true 태그 사용
            bool isLogo = body.find("\"is_logo\":true") != std::string::npos;
            std::string* pBody = new std::string(body);
            if (isLogo)
                ::PostMessage(hThis, WM_LOGO_RESPONSE, 0, (LPARAM)pBody);
            else
                ::PostMessage(hThis, WM_MENU_IMG_RESPONSE, 0, (LPARAM)pBody);
        });

    RegisterMenuCallback();

    // ★ 로고 이미지 요청
    RequestLogoImage();

    SendMenuListRequest(_T("전체"));
    return TRUE;
}

// ================================================================
//  RequestLogoImage  ★ 신규
//  m_storeInfo.logoUrl로 서버에 REQ_GET_IMAGE(217) 요청
//  응답 구분을 위해 JSON에 "is_logo":true 포함
// ================================================================
void StoreListDlg::RequestLogoImage()
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected() || m_storeInfo.logoUrl.empty()) return;

    std::string token = AuthManager::GetInstance().GetAccessToken();
    std::string json = "{\"token\":\"" + token + "\","
                       "\"image_url\":\"" + m_storeInfo.logoUrl + "\","
                       "\"is_logo\":true}";
    net.SendPacket((uint8_t)ClientType::CUSTOMER, CmdCustomer::REQ_GET_IMAGE, json);
}

// ================================================================
//  OnLogoResponse  ★ 신규  (WM_LOGO_RESPONSE)
//  로고 이미지를 IDC_STATIC_STORE_IMG 컨트롤에 표시
// ================================================================
LRESULT StoreListDlg::OnLogoResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (SLJInt(*pBody, "status") == (int)Status::SUCCESS) {
        std::string b64 = SLJStr(*pBody, "data");
        if (!b64.empty()) {
            auto bytes = ImageLoader::DecodeBase64(b64);
            HBITMAP hBmp = ImageLoader::BitmapFromBytes(bytes, 416, 107); // 컨트롤 크기에 맞춤
            if (hBmp) {
                // IDC_STATIC_STORE_IMG 컨트롤에 이미지 설정
                HBITMAP hOld = m_storeLogoCtrl.SetBitmap(hBmp);
                if (hOld) ::DeleteObject(hOld);
            }
        }
    }
    delete pBody;
    return 0;
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

// ================================================================
//  RebuildMenuListUI  ★ 수정
//  기본 회색 이미지로 먼저 표시 → RequestMenuImages로 비동기 수신
// ================================================================
void StoreListDlg::RebuildMenuListUI(const std::vector<MenuInfo>& menus, const CString& filter)
{
    m_listMenu.DeleteAllItems();

    if (m_imgListMenu.GetSafeHandle()) m_imgListMenu.DeleteImageList();
    m_imgListMenu.Create(MENU_THUMB_W, MENU_THUMB_H, ILC_COLOR32,
                         (int)menus.size() + 1, 4);
    m_listMenu.SetImageList(&m_imgListMenu, LVSIL_SMALL);

    // 기본 회색 이미지 (슬롯 0)
    {
        HDC hdcScreen = ::GetDC(nullptr);
        HDC hdc = ::CreateCompatibleDC(hdcScreen);
        HBITMAP hDef = ::CreateCompatibleBitmap(hdcScreen, MENU_THUMB_W, MENU_THUMB_H);
        HGDIOBJ hOld = ::SelectObject(hdc, hDef);
        RECT rc = {0, 0, MENU_THUMB_W, MENU_THUMB_H};
        HBRUSH hBr = ::CreateSolidBrush(RGB(210, 210, 210));
        ::FillRect(hdc, &rc, hBr); ::DeleteObject(hBr);
        ::SelectObject(hdc, hOld); ::DeleteDC(hdc);
        ::ReleaseDC(nullptr, hdcScreen);
        m_imgListMenu.Add(CBitmap::FromHandle(hDef), (CBitmap*)nullptr);
        ::DeleteObject(hDef);
    }

    int row = 0;
    int slotIdx = 1; // 슬롯 0은 기본 이미지
    m_menuImgUrlToIndex.clear();

    for (const auto& m : menus) {
        CString sub = CA2T(m.subCategory.c_str(),CP_UTF8);
        if (filter != _T("전체") && sub != filter) continue;

        // ★ 각 메뉴마다 전용 슬롯 추가 (기본 회색으로)
        {
            HDC hdcScreen = ::GetDC(nullptr);
            HDC hdc = ::CreateCompatibleDC(hdcScreen);
            HBITMAP hSlot = ::CreateCompatibleBitmap(hdcScreen, MENU_THUMB_W, MENU_THUMB_H);
            HGDIOBJ hOld = ::SelectObject(hdc, hSlot);
            RECT rc = {0, 0, MENU_THUMB_W, MENU_THUMB_H};
            HBRUSH hBr = ::CreateSolidBrush(RGB(210, 210, 210));
            ::FillRect(hdc, &rc, hBr); ::DeleteObject(hBr);
            ::SelectObject(hdc, hOld); ::DeleteDC(hdc);
            ::ReleaseDC(nullptr, hdcScreen);
            m_imgListMenu.Add(CBitmap::FromHandle(hSlot), (CBitmap*)nullptr);
            ::DeleteObject(hSlot);
        }

        // image_url → 슬롯 인덱스 맵핑
        if (!m.menuImageUrl.empty())
            m_menuImgUrlToIndex[m.menuImageUrl] = slotIdx;

        CString n = CA2T(m.menuName.c_str(),CP_UTF8);
        CString d = CA2T(m.description.c_str(),CP_UTF8);
        CString p; p.Format(_T("%d원"), m.price);

        LVITEM lvi = {};
        lvi.mask     = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem    = row++;
        lvi.iSubItem = 0;
        lvi.pszText  = (LPTSTR)(LPCTSTR)_T("");
        lvi.iImage   = slotIdx++;
        int r = m_listMenu.InsertItem(&lvi);
        m_listMenu.SetItemText(r, 1, n);
        m_listMenu.SetItemText(r, 2, d);
        m_listMenu.SetItemText(r, 3, p);
    }

    // ★ 메뉴 이미지 비동기 요청
    RequestMenuImages(menus, filter);
}

// ================================================================
//  RequestMenuImages  ★ 신규
//  각 메뉴의 image_url로 서버에 REQ_GET_IMAGE(217) 요청
// ================================================================
void StoreListDlg::RequestMenuImages(const std::vector<MenuInfo>& menus, const CString& filter)
{
    auto& net = NetworkManager::GetInstance();
    if (!net.IsConnected()) return;

    std::string token = AuthManager::GetInstance().GetAccessToken();

    for (const auto& m : menus) {
        CString sub = CA2T(m.subCategory.c_str(), CP_UTF8);
        if (filter != _T("전체") && sub != filter) continue;
        if (m.menuImageUrl.empty()) continue;

        std::string json = "{\"token\":\"" + token + "\","
                           "\"image_url\":\"" + m.menuImageUrl + "\"}";
        net.SendPacket((uint8_t)ClientType::CUSTOMER,
                       CmdCustomer::REQ_GET_IMAGE, json);
    }
}

// ================================================================
//  OnMenuImgResponse  ★ 신규  (WM_MENU_IMG_RESPONSE)
//  메뉴 이미지 base64 수신 → ImageList 슬롯 교체 → 행 갱신
// ================================================================
LRESULT StoreListDlg::OnMenuImgResponse(WPARAM, LPARAM lParam)
{
    std::string* pBody = reinterpret_cast<std::string*>(lParam);
    if (!pBody) return 0;

    if (SLJInt(*pBody, "status") == (int)Status::SUCCESS) {
        std::string imageUrl = SLJStr(*pBody, "image_url");
        std::string b64Data  = SLJStr(*pBody, "data");

        if (!imageUrl.empty() && !b64Data.empty()) {
            auto it = m_menuImgUrlToIndex.find(imageUrl);
            if (it != m_menuImgUrlToIndex.end()) {
                int imgSlot = it->second;
                auto bytes = ImageLoader::DecodeBase64(b64Data);
                HBITMAP hBmp = ImageLoader::BitmapFromBytes(bytes,
                                                            MENU_THUMB_W, MENU_THUMB_H);
                if (hBmp) {
                    m_imgListMenu.Replace(imgSlot,
                                         CBitmap::FromHandle(hBmp),
                                         (CBitmap*)nullptr);
                    ::DeleteObject(hBmp);

                    // 해당 슬롯 행 갱신
                    int count = m_listMenu.GetItemCount();
                    for (int i = 0; i < count; ++i) {
                        LVITEM lvi = {}; lvi.mask = LVIF_IMAGE;
                        lvi.iItem = i; lvi.iSubItem = 0;
                        m_listMenu.GetItem(&lvi);
                        if (lvi.iImage == imgSlot) {
                            m_listMenu.RedrawItems(i, i);
                            break;
                        }
                    }
                    m_listMenu.UpdateWindow();
                }
            }
        }
    }
    delete pBody;
    return 0;
}

void StoreListDlg::UpdateMenuListUI(CString subCat)
{
    RebuildMenuListUI(m_vecMenuCache, subCat);
}

LRESULT StoreListDlg::OnScrollMenuClicked(WPARAM wParam, LPARAM)
{
    int n = (UINT)wParam - SCROLL_MENU_BTN_ID;
    if (n >= 0 && n < (int)m_vecSubCategories.size())
        SendMenuListRequest(m_vecSubCategories[n]);
    return 0;
}

BOOL StoreListDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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

void StoreListDlg::OnBnClickedBtnStoreInfo()
{
    CMenu popup; popup.CreatePopupMenu();
    popup.AppendMenu(MF_STRING, 1, _T("가게 정보"));
    popup.AppendMenu(MF_STRING, 2, _T("리뷰 보기"));
    CPoint pt; GetCursorPos(&pt);
    int sel = popup.TrackPopupMenu(
        TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);
    if (sel == 1) {
        StoreDetailDlg dlg(this);
        dlg.m_storeInfo = m_storeInfo;
        dlg.DoModal();
    } else if (sel == 2) {
        ReviewListDlg dlg(this);
        dlg.m_storeID      = m_storeInfo.storeID;
        dlg.m_strStoreName = m_strStoreName;
        dlg.m_bCanWriteReview = false;
        dlg.DoModal();
    }
}

void StoreListDlg::OnNMClickListMenuItems(NMHDR* pNMHDR, LRESULT* pResult)
{
    int n = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR)->iItem;
    if (n >= 0 && n < (int)m_vecMenuCache.size()) {
        MenuDetailDlg dlg(this);
        dlg.m_menuInfo = m_vecMenuCache[n];
        dlg.DoModal();
    }
    *pResult = 0;
}

HBITMAP StoreListDlg::LoadMenuImage(const CString& relPath)
{
    CString fullPath = ImageLoader::MakeServerPath(relPath);
    return ImageLoader::Load(fullPath);
}
void StoreListDlg::ResizeBitmapTo(HBITMAP& hBmp, int w, int h) { (void)hBmp; (void)w; (void)h; }
