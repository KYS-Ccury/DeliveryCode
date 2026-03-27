/**
 * PageDispatch.cpp
 * ============================================================
 * ★ 수정:
 *   1) UTF-8 → CString 변환 (Utf8ToCString) 적용 → 한글 깨짐 해결
 *   2) 상태 변경 시 서버에 패킷 송신 추가
 * ============================================================
 */

#include "pch.h"
#include "PageDispatch.h"
#include "admintool.h"
#include "PacketDef.h"
#include "PageInquiry.h"    // ★ Utf8ToCString, CStringToUtf8 헬퍼

IMPLEMENT_DYNAMIC(PageDispatch, PageBase)

static CClientSocket& GetSocket()
{
    return ((CAdminToolApp*)AfxGetApp())->GetSocket();
}

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
            rcClient.right, rcClient.bottom - rcBtn.bottom - 2, SWP_NOZORDER);
    }
    else
    {
        m_listDispatch.SetWindowPos(nullptr, 0, 0,
            rcClient.right, rcClient.bottom, SWP_NOZORDER);
    }

    CRect rcList;
    m_listDispatch.GetClientRect(&rcList);
    int nW = rcList.Width() - 4;
    if (nW <= 0) return;
    m_listDispatch.SetColumnWidth(0, (int)(nW * 0.35));
    m_listDispatch.SetColumnWidth(1, (int)(nW * 0.35));
    m_listDispatch.SetColumnWidth(2, (int)(nW * 0.30));
}

// ============================================================
// ★ 서버에서 주문 모니터링 데이터 로드 (UTF-8 변환 적용)
// ============================================================
void PageDispatch::LoadDataFromServer()
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    json reqBody;
    if (!sock.SendAdminPacket(CMD_ORDER_MONITOR, reqBody)) return;

    RecvResult res = sock.RecvPacket();
    if (!res.success) return;

    m_listDispatch.DeleteAllItems();

    if (res.body.contains("orders") && res.body["orders"].is_array())
    {
        int nRow = 0;
        for (auto& order : res.body["orders"])
        {
            // ★ UTF-8 → CString 변환
            CString strOrderId = Utf8ToCString(order.value("order_id", ""));
            CString strRiderName = Utf8ToCString(order.value("rider_name", ""));
            CString strStatus = Utf8ToCString(order.value("status", ""));

            int idx = m_listDispatch.InsertItem(nRow, strOrderId);
            m_listDispatch.SetItemText(idx, 1, strRiderName);
            m_listDispatch.SetItemText(idx, 2, strStatus);
            nRow++;
        }
    }
}

void PageDispatch::RequestForceDispatch(int orderId, int riderId)
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    json reqBody;
    reqBody["order_id"] = orderId;
    reqBody["rider_id"] = riderId;

    if (!sock.SendAdminPacket(CMD_FORCE_DISPATCH, reqBody))
    {
        AfxMessageBox(_T("강제 배차 요청 실패"));
        return;
    }

    RecvResult res = sock.RecvPacket();
    if (res.success && res.body.contains("status")
        && res.body["status"].get<int>() == STATUS_SUCCESS)
    {
        AfxMessageBox(_T("강제 배차 완료"));
        LoadDataFromServer();
    }
    else
    {
        AfxMessageBox(_T("강제 배차 실패"));
    }
}

void PageDispatch::RequestForceCancel(int orderId)
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    json reqBody;
    reqBody["order_id"] = orderId;

    if (!sock.SendAdminPacket(CMD_FORCE_CANCEL, reqBody))
    {
        AfxMessageBox(_T("배차 취소 요청 실패"));
        return;
    }

    RecvResult res = sock.RecvPacket();
    if (res.success && res.body.contains("status")
        && res.body["status"].get<int>() == STATUS_SUCCESS)
    {
        AfxMessageBox(_T("배차 취소 완료"));
        LoadDataFromServer();
    }
    else
    {
        AfxMessageBox(_T("배차 취소 실패"));
    }
}

// ============================================================
// ★ 상태 변경을 서버에 전송
// ============================================================
void PageDispatch::RequestStatusChange(int nItemIndex, const CString& strNewStatus)
{
    CString strOrderId = m_listDispatch.GetItemText(nItemIndex, 0);

    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected())
    {
        AfxMessageBox(_T("서버에 연결되어 있지 않습니다."));
        return;
    }

    json reqBody;
    reqBody["order_id"] = CStringToUtf8(strOrderId);   // ★ UTF-8 변환
    reqBody["status"] = CStringToUtf8(strNewStatus);  // ★ UTF-8 변환

    if (!sock.SendAdminPacket(CMD_FORCE_DISPATCH, reqBody))
    {
        AfxMessageBox(_T("상태 변경 요청 실패"));
        return;
    }

    RecvResult res = sock.RecvPacket();
    if (!res.success || !res.body.contains("status")
        || res.body["status"].get<int>() != STATUS_SUCCESS)
    {
        AfxMessageBox(_T("상태 변경 실패 - 목록을 새로고침합니다."));
        LoadDataFromServer();
    }
}

void PageDispatch::LoadData()
{
    RepositionList();
    LoadDataFromServer();
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
    if (nSel == -1) { AfxMessageBox(_T("항목을 선택하세요")); return; }

    CString strOrderId = m_listDispatch.GetItemText(nSel, 0);
    int orderId = _ttoi(strOrderId);
    if (orderId <= 0)
    {
        AfxMessageBox(_T("주문 ID 형식 확인 필요"));
        return;
    }
    RequestForceCancel(orderId);
}

// ★ 상태 컬럼 클릭 → 서버에도 전송
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
    int nCmd = (int)menu.TrackPopupMenu(
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        pt.x, pt.y, this);

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

    // ★ 서버에도 상태 변경 전송
    RequestStatusChange(pNMLV->iItem, s);
}

void PageDispatch::OnCustomDrawDispatch(NMHDR* pNMHDR, LRESULT* pResult)
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
    if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && pLVCD->iSubItem == 2)
    {
        CString s = m_listDispatch.GetItemText((int)pLVCD->nmcd.dwItemSpec, 2);
        if (s == _T("완료") || s == _T("DONE"))             pLVCD->clrText = RGB(0, 150, 0);
        else if (s == _T("대기중") || s == _T("PENDING"))    pLVCD->clrText = RGB(255, 140, 0);
        else if (s == _T("배송중") || s == _T("DELIVERING")) pLVCD->clrText = RGB(200, 0, 0);
        else if (s == _T("배차완료") || s == _T("ACCEPTED")) pLVCD->clrText = RGB(128, 0, 128);
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