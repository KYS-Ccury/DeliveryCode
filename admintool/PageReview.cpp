#include "pch.h"
#include "PageReview.h"

IMPLEMENT_DYNAMIC(PageReview, PageBase)

PageReview::PageReview(CWnd* pParent)
    : PageBase(IDD_PAGE_REVIEW, pParent)
{
}

PageReview::~PageReview()
{
    if (m_imgList.GetSafeHandle())
        m_imgList.DeleteImageList();
}

void PageReview::DoDataExchange(CDataExchange* pDX)
{
    PageBase::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_REVIEW, m_listReview);
}

BOOL PageReview::OnInitDialog()
{
    PageBase::OnInitDialog();
    m_font.CreatePointFont(110, _T("맑은 고딕"));
    InitListCtrl();
    return TRUE;
}

void PageReview::InitListCtrl()
{
    m_imgList.Create(1, 24, ILC_COLOR, 0, 1);
    m_listReview.SetImageList(&m_imgList, LVSIL_SMALL);
    m_listReview.SetExtendedStyle(
        m_listReview.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT
    );
    m_listReview.SetFont(&m_font);
    m_listReview.InsertColumn(0, _T("No"), LVCFMT_CENTER, 50);
    m_listReview.InsertColumn(1, _T("ID"), LVCFMT_LEFT, 120);
    m_listReview.InsertColumn(2, _T("Content"), LVCFMT_LEFT, 300);
    m_listReview.InsertColumn(3, _T("Status"), LVCFMT_CENTER, 100);
}

void PageReview::RepositionList()
{
    if (!IsWindow(m_listReview.GetSafeHwnd())) return;
    CRect rcClient;
    GetClientRect(&rcClient);
    if (rcClient.Width() <= 0 || rcClient.Height() <= 0) return;

    CRect rcBtn;
    CWnd* pBtn = GetDlgItem(IDC_BTN_REVIEW_DEL);
    if (pBtn && IsWindow(pBtn->GetSafeHwnd()))
    {
        pBtn->GetWindowRect(&rcBtn);
        ScreenToClient(&rcBtn);
        m_listReview.SetWindowPos(nullptr, 0, rcBtn.bottom + 2,
            rcClient.right, rcClient.bottom - rcBtn.bottom - 2,
            SWP_NOZORDER);
    }
    else
    {
        m_listReview.SetWindowPos(nullptr, 0, 0,
            rcClient.right, rcClient.bottom,
            SWP_NOZORDER);
    }

    CRect rcList;
    m_listReview.GetClientRect(&rcList);
    int nW = rcList.Width() - 4;
    if (nW <= 0) return;
    m_listReview.SetColumnWidth(0, (int)(nW * 0.08));
    m_listReview.SetColumnWidth(1, (int)(nW * 0.20));
    m_listReview.SetColumnWidth(2, (int)(nW * 0.57));
    m_listReview.SetColumnWidth(3, (int)(nW * 0.15));
}

void PageReview::InsertDummyData()
{
    m_listReview.DeleteAllItems();
    int nRow;
    nRow = m_listReview.InsertItem(0, _T("1")); m_listReview.SetItemText(nRow, 1, _T("user001")); m_listReview.SetItemText(nRow, 2, _T("Fast delivery, fresh food"));   m_listReview.SetItemText(nRow, 3, _T("보이기"));
    nRow = m_listReview.InsertItem(1, _T("2")); m_listReview.SetItemText(nRow, 1, _T("user002")); m_listReview.SetItemText(nRow, 2, _T("Bad packaging"));                 m_listReview.SetItemText(nRow, 3, _T("숨김"));
    nRow = m_listReview.InsertItem(2, _T("3")); m_listReview.SetItemText(nRow, 1, _T("user003")); m_listReview.SetItemText(nRow, 2, _T("Kind rider, good experience"));    m_listReview.SetItemText(nRow, 3, _T("보이기"));
    nRow = m_listReview.InsertItem(3, _T("4")); m_listReview.SetItemText(nRow, 1, _T("user004")); m_listReview.SetItemText(nRow, 2, _T("Food was cold on arrival"));       m_listReview.SetItemText(nRow, 3, _T("숨김"));
    nRow = m_listReview.InsertItem(4, _T("5")); m_listReview.SetItemText(nRow, 1, _T("user005")); m_listReview.SetItemText(nRow, 2, _T("Wrong amount charged"));           m_listReview.SetItemText(nRow, 3, _T("보이기"));
}

void PageReview::LoadData()
{
    RepositionList();
    InsertDummyData();
}

void PageReview::SaveData() { AfxMessageBox(_T("Review Saved")); }

void PageReview::OnSize(UINT nType, int cx, int cy)
{
    PageBase::OnSize(nType, cx, cy);
    RepositionList();
}

void PageReview::OnBtnReviewDel()
{
    int nSel = m_listReview.GetNextItem(-1, LVNI_SELECTED);
    if (nSel == -1) { AfxMessageBox(_T("Select an item")); return; }
    m_listReview.DeleteItem(nSel);
}

void PageReview::OnListItemClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;
    if (pNMLV->iSubItem != 3 || pNMLV->iItem < 0) return;

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 2001, _T("보이기"));
    menu.AppendMenu(MF_STRING, 2002, _T("숨김"));
    POINT pt; GetCursorPos(&pt);
    int nCmd = (int)menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, this);
    if (nCmd == 2001) m_listReview.SetItemText(pNMLV->iItem, 3, _T("보이기"));
    else if (nCmd == 2002) m_listReview.SetItemText(pNMLV->iItem, 3, _T("숨김"));
    m_listReview.Invalidate();
}

void PageReview::OnCustomDrawReview(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT) { *pResult = CDRF_NOTIFYITEMDRAW;    return; }
    if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) { *pResult = CDRF_NOTIFYSUBITEMDRAW; return; }
    if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && pLVCD->iSubItem == 3)
    {
        CString s = m_listReview.GetItemText((int)pLVCD->nmcd.dwItemSpec, 3);
        if (s == _T("보이기")) pLVCD->clrText = RGB(0, 150, 0);
        else if (s == _T("숨김"))   pLVCD->clrText = RGB(180, 0, 0);
        *pResult = CDRF_NEWFONT;
        return;
    }
    *pResult = CDRF_DODEFAULT;
}

BEGIN_MESSAGE_MAP(PageReview, PageBase)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_REVIEW_DEL, &PageReview::OnBtnReviewDel)
    ON_NOTIFY(NM_CLICK, IDC_LIST_REVIEW, &PageReview::OnListItemClick)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_REVIEW, &PageReview::OnCustomDrawReview)
END_MESSAGE_MAP()