/**
 * PageReview.cpp
 * ============================================================
 * ★ 수정사항:
 *   1) 서버 요청(520)으로 리뷰 CRUD
 *   2) ★ 소켓 Lock/Unlock 추가 (폴링 스레드와 경합 방지)
 * ============================================================
 */

#include "pch.h"
#include "PageReview.h"
#include "admintool.h"
#include "PacketDef.h"

IMPLEMENT_DYNAMIC(PageReview, PageBase)

static CClientSocket& GetSocket()
{
    return ((CAdminToolApp*)AfxGetApp())->GetSocket();
}

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

// ============================================================
// ★ 서버에서 리뷰 목록 로드 (Lock/Unlock 추가)
// ============================================================
void PageReview::LoadDataFromServer()
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected())
    {
        AfxMessageBox(_T("서버에 연결되어 있지 않습니다."));
        return;
    }

    sock.Lock();  // ★

    json reqBody;
    reqBody["action"] = "list";

    if (!sock.SendAdminPacket(CMD_MANAGE_REVIEW, reqBody))
    {
        sock.Unlock();
        AfxMessageBox(_T("리뷰 목록 요청 실패"));
        return;
    }

    RecvResult res = sock.RecvPacket();

    sock.Unlock();  // ★

    if (!res.success)
    {
        CString msg;
        msg.Format(_T("응답 수신 실패: %S"), sock.GetLastErrorMsg().c_str());
        AfxMessageBox(msg);
        return;
    }

    m_listReview.DeleteAllItems();

    if (res.body.contains("reviews") && res.body["reviews"].is_array())
    {
        int nRow = 0;
        for (auto& review : res.body["reviews"])
        {
            std::string reviewId = std::to_string(review.value("review_id", 0));
            std::string userId = review.value("user_id", "");
            std::string content = review.value("content", "");
            bool visible = review.value("visible", true);

            int idx = m_listReview.InsertItem(nRow, CString(reviewId.c_str()));
            m_listReview.SetItemText(idx, 1, CString(userId.c_str()));
            m_listReview.SetItemText(idx, 2, CString(content.c_str()));
            m_listReview.SetItemText(idx, 3, visible ? _T("보이기") : _T("숨김"));
            nRow++;
        }
    }
}

// ============================================================
// ★ 리뷰 삭제 요청 (Lock/Unlock 추가)
// ============================================================
void PageReview::RequestDeleteReview(int reviewId)
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    sock.Lock();  // ★

    json reqBody;
    reqBody["action"] = "delete";
    reqBody["review_id"] = reviewId;

    if (!sock.SendAdminPacket(CMD_MANAGE_REVIEW, reqBody))
    {
        sock.Unlock();
        AfxMessageBox(_T("리뷰 삭제 요청 실패"));
        return;
    }

    RecvResult res = sock.RecvPacket();

    sock.Unlock();  // ★

    if (res.success && res.body.contains("status")
        && res.body["status"].get<int>() == STATUS_SUCCESS)
    {
        AfxMessageBox(_T("리뷰 삭제 완료"));
        LoadDataFromServer();
    }
    else
    {
        AfxMessageBox(_T("리뷰 삭제 실패"));
    }
}

void PageReview::LoadData()
{
    RepositionList();
    LoadDataFromServer();
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
    if (nSel == -1) { AfxMessageBox(_T("항목을 선택하세요")); return; }

    CString strReviewId = m_listReview.GetItemText(nSel, 0);
    int reviewId = _ttoi(strReviewId);
    if (reviewId <= 0)
    {
        AfxMessageBox(_T("리뷰 ID 형식 확인 필요"));
        return;
    }

    RequestDeleteReview(reviewId);
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
    int nCmd = (int)menu.TrackPopupMenu(
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        pt.x, pt.y, this);

    if (nCmd == 2001)
        m_listReview.SetItemText(pNMLV->iItem, 3, _T("보이기"));
    else if (nCmd == 2002)
        m_listReview.SetItemText(pNMLV->iItem, 3, _T("숨김"));
    m_listReview.Invalidate();

    // ★ TODO: 상태 변경 시 서버에도 반영
    //    json reqBody;
    //    reqBody["action"] = "toggle_visibility";
    //    reqBody["review_id"] = reviewId;
    //    reqBody["visible"] = (nCmd == 2001);
    //    sock.SendAdminPacket(CMD_MANAGE_REVIEW, reqBody);
}

void PageReview::OnCustomDrawReview(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        *pResult = CDRF_NOTIFYITEMDRAW; return;
    }
    if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        *pResult = CDRF_NOTIFYSUBITEMDRAW; return;
    }
    if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)
        && pLVCD->iSubItem == 3)
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