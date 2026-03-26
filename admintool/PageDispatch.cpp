#include "pch.h"
#include "PageDispatch.h"

IMPLEMENT_DYNAMIC(PageDispatch, PageBase)

PageDispatch::PageDispatch(CWnd* pParent)
    : PageBase(IDD_PAGE_DISPATCH, pParent)
{
}

PageDispatch::~PageDispatch()
{
    if (m_imgList.GetSafeHandle())
        m_imgList.DeleteImageList();
}

void PageDispatch::DoDataExchange(CDataExchange* pDX)
{
    PageBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_DISPATCH, m_listDispatch);
}

BOOL PageDispatch::OnInitDialog()
{
    PageBase::OnInitDialog();
    m_font.CreatePointFont(110, _T("맑은 고딕"));
    InitListCtrl();
    return TRUE;
}

void PageDispatch::InitListCtrl()
{
    m_imgList.Create(1, 24, ILC_COLOR, 0, 1);
    m_listDispatch.SetImageList(&m_imgList, LVSIL_SMALL);
    m_listDispatch.SetExtendedStyle(
        m_listDispatch.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT
    );
    m_listDispatch.SetFont(&m_font);
    m_listDispatch.InsertColumn(0, _T("OrderID"), LVCFMT_LEFT, 150);
    m_listDispatch.InsertColumn(1, _T("Rider"), LVCFMT_LEFT, 150);
    m_listDispatch.InsertColumn(2, _T("Status"), LVCFMT_CENTER, 120);
}

void PageDispatch::RepositionList()
{
    if (!IsWindow(m_listDispatch.GetSafeHwnd())) return;
    CRect rcClient;
    GetClientRect(&rcClient);
    if (rcClient.Width() <= 0 || rcClient.Height() <= 0) return;

    CRect rcBtn;
    CWnd* pBtn = GetDlgItem(IDC_BTN_DISPATCH_DEL);
    if (pBtn && IsWindow(pBtn->GetSafeHwnd()))
    {
        pBtn->GetWindowRect(&rcBtn);
        ScreenToClient(&rcBtn);
        m_listDispatch.SetWindowPos(nullptr, 0, rcBtn.bottom + 2,
            rcClient.right, rcClient.bottom - rcBtn.bottom - 2,
            SWP_NOZORDER);
    }
    else
    {
        m_listDispatch.SetWindowPos(nullptr, 0, 0,
            rcClient.right, rcClient.bottom,
            SWP_NOZORDER);
    }

    CRect rcList;
    m_listDispatch.GetClientRect(&rcList);
    int nW = rcList.Width() - 4;
    if (nW <= 0) return;
    m_listDispatch.SetColumnWidth(0, (int)(nW * 0.35));
    m_listDispatch.SetColumnWidth(1, (int)(nW * 0.35));
    m_listDispatch.SetColumnWidth(2, (int)(nW * 0.30));
}

void PageDispatch::InsertDummyData()
{
    m_listDispatch.DeleteAllItems();
    int nRow;
    nRow = m_listDispatch.InsertItem(0, _T("ORD-20240001")); m_listDispatch.SetItemText(nRow, 1, _T("Kim"));  m_listDispatch.SetItemText(nRow, 2, _T("대기중"));
    nRow = m_listDispatch.InsertItem(1, _T("ORD-20240002")); m_listDispatch.SetItemText(nRow, 1, _T("Lee"));  m_listDispatch.SetItemText(nRow, 2, _T("배차완료"));
    nRow = m_listDispatch.InsertItem(2, _T("ORD-20240003")); m_listDispatch.SetItemText(nRow, 1, _T("Park")); m_listDispatch.SetItemText(nRow, 2, _T("대기중"));
    nRow = m_listDispatch.InsertItem(3, _T("ORD-20240004")); m_listDispatch.SetItemText(nRow, 1, _T("Choi")); m_listDispatch.SetItemText(nRow, 2, _T("배송중"));
    nRow = m_listDispatch.InsertItem(4, _T("ORD-20240005")); m_listDispatch.SetItemText(nRow, 1, _T("Jung")); m_listDispatch.SetItemText(nRow, 2, _T("완료"));
}

void PageDispatch::LoadData()
{
    RepositionList();
    InsertDummyData();
}

void PageDispatch::SaveData() { AfxMessageBox(_T("Dispatch Saved")); }

void PageDispatch::OnSize(UINT nType, int cx, int cy)
{
    PageBase::OnSize(nType, cx, cy);
    RepositionList();
}

void PageDispatch::OnBtnDispatchDel()
{
    int nSel = m_listDispatch.GetNextItem(-1, LVNI_SELECTED);
    if (nSel == -1) { AfxMessageBox(_T("Select an item")); return; }
    m_listDispatch.DeleteItem(nSel);
}

void PageDispatch::OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;
    if (pNMLV->iSubItem != 2 || pNMLV->iItem < 0) return;

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1001, _T("대기중"));
    menu.AppendMenu(MF_STRING, 1002, _T("배차완료"));
    menu.AppendMenu(MF_STRING, 1003, _T("배송중"));
    menu.AppendMenu(MF_STRING, 1004, _T("완료"));
    POINT pt; GetCursorPos(&pt);
    int nCmd = (int)menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, this);
    CString s;
    switch (nCmd)
    {
    case 1001: s = _T("대기중");   break;
    case 1002: s = _T("배차완료"); break;
    case 1003: s = _T("배송중");   break;
    case 1004: s = _T("완료");     break;
    default: return;
    }
    m_listDispatch.SetItemText(pNMLV->iItem, 2, s);
    m_listDispatch.Invalidate();
}

void PageDispatch::OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT) { *pResult = CDRF_NOTIFYITEMDRAW;    return; }
    if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) { *pResult = CDRF_NOTIFYSUBITEMDRAW; return; }
    if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && pLVCD->iSubItem == 2)
    {
        CString s = m_listDispatch.GetItemText((int)pLVCD->nmcd.dwItemSpec, 2);
        if (s == _T("완료"))     pLVCD->clrText = RGB(0, 150, 0);
        else if (s == _T("대기중"))   pLVCD->clrText = RGB(255, 140, 0);
        else if (s == _T("배송중"))   pLVCD->clrText = RGB(200, 0, 0);
        else if (s == _T("배차완료")) pLVCD->clrText = RGB(128, 0, 128);
        *pResult = CDRF_NEWFONT;
        return;
    }
    *pResult = CDRF_DODEFAULT;
}

BEGIN_MESSAGE_MAP(PageDispatch, PageBase)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_DISPATCH_DEL, &PageDispatch::OnBtnDispatchDel)
    ON_NOTIFY(NM_CLICK, IDC_LIST_DISPATCH, &PageDispatch::OnListItemClick)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_DISPATCH, &PageDispatch::OnCustomDrawDispatch)
END_MESSAGE_MAP()