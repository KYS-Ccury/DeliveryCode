// DeliveryPhotoDlg.cpp - Delivery complete with JSON protocol
#include "pch.h"
#include "DeliveryPhotoDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

IMPLEMENT_DYNAMIC(DeliveryPhotoDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DeliveryPhotoDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_SELECT_PHOTO,  &DeliveryPhotoDlg::OnBtnSelectPhoto)
    ON_BN_CLICKED(IDC_BTN_SKIP_PHOTO,    &DeliveryPhotoDlg::OnBtnSkipPhoto)
    ON_BN_CLICKED(IDC_BTN_CONFIRM_PHOTO, &DeliveryPhotoDlg::OnBtnConfirmPhoto)
    ON_MESSAGE(WM_SOCKET_RECV,           &DeliveryPhotoDlg::OnSocketRecv)
END_MESSAGE_MAP()

DeliveryPhotoDlg::DeliveryPhotoDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_PHOTO_DLG, pParent) {}
DeliveryPhotoDlg::~DeliveryPhotoDlg() {}

void DeliveryPhotoDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }

BOOL DeliveryPhotoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(FALSE);
    return TRUE;
}

void DeliveryPhotoDlg::OnBtnSelectPhoto()
{
    CFileDialog dlg(TRUE, _T("jpg"), nullptr,
                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("이미지 파일 (*.jpg;*.jpeg;*.png)|*.jpg;*.jpeg;*.png|모든 파일 (*.*)|*.*||"), this);
    if (dlg.DoModal() == IDOK) {
        m_strPhotoPath = dlg.GetPathName();
        SetDlgItemText(IDC_STATIC_PHOTO_PATH, m_strPhotoPath);
        SetDlgItemText(IDC_STATIC_QR_VIEW, _T("[사진 첨부됨]"));
        GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
    }
}

void DeliveryPhotoDlg::OnBtnSkipPhoto()
{
    m_strPhotoPath.Empty();
    SetDlgItemText(IDC_STATIC_PHOTO_PATH, _T("(사진 없음)"));
    SetDlgItemText(IDC_STATIC_QR_VIEW, _T("(사진 없이 완료)"));
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
}

// Send: JSON {"order_id": N}
void DeliveryPhotoDlg::OnBtnConfirmPhoto()
{
    int orderId = AppContext::Get().currentOrder.orderId;
    if (orderId <= 0) {
        MessageBox(_T("유효하지 않은 주문입니다."), _T("오류"), MB_OK | MB_ICONWARNING);
        return;
    }
    json req;
    req["order_id"] = orderId;
    if (!m_strPhotoPath.IsEmpty()) {
        CT2A pathUtf8(m_strPhotoPath, CP_UTF8);
        req["photo_path"] = std::string(pathUtf8);
    }

    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_DELIVERY_DONE, req.dump());
    if (!bSent) {
        MessageBox(_T("배달이 완료되었습니다! 수고하셨습니다."), _T("완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
}

// Recv: {"status":2000,"delivery_fee":3000,"message":"..."}
LRESULT DeliveryPhotoDlg::OnSocketRecv(WPARAM, LPARAM lParam)
{
    RecvPacket* pPkt = reinterpret_cast<RecvPacket*>(lParam);
    if (!pPkt) return 0;
    UINT16      protocol = pPkt->protocol;
    std::string body     = pPkt->body;
    delete pPkt;

    if (protocol != CMD_RIDER_DELIVERY_DONE) return 0;

    try {
        json res = json::parse(body);
        if (res.value("status", 0) == STATUS_SUCCESS) {
            std::string msg = res.value("message", "Delivery completed! Great job.");
            CA2T wMsg(msg.c_str(), CP_UTF8);
            MessageBox(CString(wMsg), _T("완료"), MB_OK | MB_ICONINFORMATION);
            EndDialog(IDOK);
        } else {
            std::string err = res.value("message", "Server error. Please retry.");
            CA2T wErr(err.c_str(), CP_UTF8);
            MessageBox(CString(wErr), _T("오류"), MB_OK | MB_ICONWARNING);
            GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
        }
    } catch (...) {}
    return 0;
}

HBRUSH DeliveryPhotoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg)
            m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    // Buttons: do NOT override - let Windows draw button text normally
    return hbr;
}
