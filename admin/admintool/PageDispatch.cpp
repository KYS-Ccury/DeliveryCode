/**
 * PageDispatch.cpp
 * ============================================================
 * ★ 수정사항: InsertDummyData() → 서버 요청으로 교체
 *   510: 대기 주문 모니터링 (LoadData에서 호출)
 *   511: 라이더 현황
 *   512: 강제 배차 (상태 변경 팝업에서)
 *   513: 배차 강제 취소 (삭제 버튼에서)
 * ============================================================
 */

#include "pch.h"
#include "PageDispatch.h"
#include "admintool.h"      // ★ 추가: GetSocket()
#include "PacketDef.h"      // ★ 추가: CMD 상수

IMPLEMENT_DYNAMIC(PageDispatch, PageBase)

// ★ 헬퍼: App에서 소켓 가져오기
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

// ============================================================
// ★ 서버에서 주문 모니터링 데이터 로드 (CMD_ORDER_MONITOR = 510)
// ============================================================
void PageDispatch::LoadDataFromServer()
{
    CClientSocket& sock = GetSocket();

    // 연결 확인
    if (!sock.IsConnected())
    {
        AfxMessageBox(_T("서버에 연결되어 있지 않습니다."));
        return;
    }

    // 요청 송신 (510: 대기 주문 모니터링)
    json reqBody;  // 빈 JSON (전체 조회)
    if (!sock.SendAdminPacket(CMD_ORDER_MONITOR, reqBody))
    {
        AfxMessageBox(_T("주문 모니터링 요청 실패"));
        return;
    }

    // 응답 수신
    RecvResult res = sock.RecvPacket();
    if (!res.success)
    {
        CString msg;
        msg.Format(_T("응답 수신 실패: %S"), sock.GetLastErrorMsg().c_str());
        AfxMessageBox(msg);
        return;
    }

    // ★ TODO: 서버 AdminDB.cpp 응답 JSON 스키마 확인 후 수정
    // 예상 응답: { "status": 2000, "orders": [ { "order_id": 1, "rider_name": "Kim", "status": "대기중" }, ... ] }

    // 리스트 초기화
    m_listDispatch.DeleteAllItems();

    if (res.body.contains("orders") && res.body["orders"].is_array())
    {
        int nRow = 0;
        for (auto& order : res.body["orders"])
        {
            // 각 필드 추출 (TODO: 서버 필드명 확인)
            std::string orderId = order.value("order_id", "");
            std::string riderName = order.value("rider_name", "");
            std::string status = order.value("status", "");

            // 리스트에 추가
            int idx = m_listDispatch.InsertItem(nRow, CString(orderId.c_str()));
            m_listDispatch.SetItemText(idx, 1, CString(riderName.c_str()));
            m_listDispatch.SetItemText(idx, 2, CString(status.c_str()));
            nRow++;
        }
    }
}

// ============================================================
// ★ 강제 배차 요청 (CMD_FORCE_DISPATCH = 512)
// ============================================================
void PageDispatch::RequestForceDispatch(int orderId, int riderId)
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    json reqBody;
    reqBody["order_id"] = orderId;   // TODO: 서버 필드명 확인
    reqBody["rider_id"] = riderId;   // TODO: 서버 필드명 확인

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
        LoadDataFromServer();  // 목록 새로고침
    }
    else
    {
        AfxMessageBox(_T("강제 배차 실패"));
    }
}

// ============================================================
// ★ 배차 강제 취소 요청 (CMD_FORCE_CANCEL = 513)
// ============================================================
void PageDispatch::RequestForceCancel(int orderId)
{
    CClientSocket& sock = GetSocket();
    if (!sock.IsConnected()) return;

    json reqBody;
    reqBody["order_id"] = orderId;   // TODO: 서버 필드명 확인

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
        LoadDataFromServer();  // 목록 새로고침
    }
    else
    {
        AfxMessageBox(_T("배차 취소 실패"));
    }
}

// ============================================================
// LoadData: 페이지 진입 시 호출 (기존 InsertDummyData → 서버 요청)
// ============================================================
void PageDispatch::LoadData()
{
    RepositionList();
    LoadDataFromServer();   // ★ 더미 → 서버
}

void PageDispatch::SaveData() { AfxMessageBox(_T("Dispatch Saved")); }

void PageDispatch::OnSize(UINT nType, int cx, int cy)
{
    PageBase::OnSize(nType, cx, cy);
    RepositionList();
}

// ★ 삭제 버튼 → 배차 강제 취소 (513)
void PageDispatch::OnBtnDispatchDel()
{
    int nSel = m_listDispatch.GetNextItem(-1, LVNI_SELECTED);
    if (nSel == -1) { AfxMessageBox(_T("항목을 선택하세요")); return; }

    // 선택된 행에서 OrderID 추출
    CString strOrderId = m_listDispatch.GetItemText(nSel, 0);

    // ★ TODO: OrderID를 정수로 변환 (서버 형식에 맞게)
    //    현재 서버 응답 형식 미확인으로, 문자열 → 정수 변환 시도
    int orderId = _ttoi(strOrderId);
    if (orderId <= 0)
    {
        // 숫자가 아닌 경우 (예: "ORD-20240001") → 서버 형식 확인 필요
        AfxMessageBox(_T("주문 ID 형식 확인 필요"));
        return;
    }

    RequestForceCancel(orderId);
}

// 상태 컬럼 클릭 → 팝업 메뉴 (기존과 동일 + 서버 연동 가능)
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

    // UI 즉시 반영
    m_listDispatch.SetItemText(pNMLV->iItem, 2, s);
    m_listDispatch.Invalidate();

    // ★ TODO: 상태 변경 시 서버에도 반영하려면 여기서 패킷 송신
    //    예: 강제 배차(512)나 별도 상태 변경 프로토콜 호출
}

// 커스텀 드로우 (기존과 동일 - 색상 표시)
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
    if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)
        && pLVCD->iSubItem == 2)
    {
        CString s = m_listDispatch.GetItemText((int)pLVCD->nmcd.dwItemSpec, 2);
        if (s == _T("완료"))       pLVCD->clrText = RGB(0, 150, 0);
        else if (s == _T("대기중"))     pLVCD->clrText = RGB(255, 140, 0);
        else if (s == _T("배송중"))     pLVCD->clrText = RGB(200, 0, 0);
        else if (s == _T("배차완료"))   pLVCD->clrText = RGB(128, 0, 128);
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