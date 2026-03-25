// DeliveryPhotoDlg.cpp - Photo preview with GDI+
#include "pch.h"
#include "DeliveryPhotoDlg.h"
#include "Protocol.h"
#include "AppContext.h"
#include "json.hpp"
using json = nlohmann::json;

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

IMPLEMENT_DYNAMIC(DeliveryPhotoDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DeliveryPhotoDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_BN_CLICKED(IDC_BTN_SELECT_PHOTO,  &DeliveryPhotoDlg::OnBtnSelectPhoto)
    ON_BN_CLICKED(IDC_BTN_SKIP_PHOTO,    &DeliveryPhotoDlg::OnBtnSkipPhoto)
    ON_BN_CLICKED(IDC_BTN_CONFIRM_PHOTO, &DeliveryPhotoDlg::OnBtnConfirmPhoto)
    ON_MESSAGE(WM_SOCKET_RECV,           &DeliveryPhotoDlg::OnSocketRecv)
END_MESSAGE_MAP()

DeliveryPhotoDlg::DeliveryPhotoDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_PHOTO_DLG, pParent)
    , m_bHasPhoto(false)
    , m_gdiplusToken(0)
{
    // Store parent HWND now - GetParent() is unsafe in destructor
    m_hParentWnd = (pParent && IsWindow(pParent->GetSafeHwnd()))
                   ? pParent->GetSafeHwnd() : nullptr;

    GdiplusStartupInput input;
    GdiplusStartup(&m_gdiplusToken, &input, nullptr);
}

DeliveryPhotoDlg::~DeliveryPhotoDlg()
{
    if (m_bitmap.GetSafeHandle())
        m_bitmap.DeleteObject();
    if (m_gdiplusToken)
        GdiplusShutdown(m_gdiplusToken);

    // Restore CMD_RIDER_DELIVERY_DONE routing to parent (MainDlg)
    // Use pre-stored HWND - GetParent() is UNSAFE in destructor (MFC ASSERT)
    if (m_hParentWnd && IsWindow(m_hParentWnd))
        AppContext::Get().socket.RegisterWnd(CMD_RIDER_DELIVERY_DONE, m_hParentWnd);
    else
        AppContext::Get().socket.UnregisterWnd(CMD_RIDER_DELIVERY_DONE);
}

void DeliveryPhotoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL DeliveryPhotoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    // Register this dialog for delivery done response
    // (overrides MainDlg's registration while this dialog is open)
    AppContext::Get().socket.RegisterWnd(CMD_RIDER_DELIVERY_DONE, GetSafeHwnd());
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(FALSE);
    SetDlgItemText(IDC_STATIC_PHOTO_PATH, _T(""));
    return TRUE;
}

void DeliveryPhotoDlg::LoadAndShowPhoto(const CString& path)
{
    if (m_bitmap.GetSafeHandle())
        m_bitmap.DeleteObject();
    m_bHasPhoto = false;

    CWnd* pPreviewWnd = GetDlgItem(IDC_STATIC_QR_VIEW);
    if (!pPreviewWnd) return;

    CRect rcPreview;
    pPreviewWnd->GetClientRect(&rcPreview);
    int dstW = rcPreview.Width();
    int dstH = rcPreview.Height();
    if (dstW <= 0 || dstH <= 0) return;

    CT2W pathW(path);
    Gdiplus::Image* pImg = Gdiplus::Image::FromFile(pathW);
    if (!pImg || pImg->GetLastStatus() != Ok) {
        delete pImg;
        SetDlgItemText(IDC_STATIC_QR_VIEW, _T("[Load failed]"));
        return;
    }

    int srcW = static_cast<int>(pImg->GetWidth());
    int srcH = static_cast<int>(pImg->GetHeight());
    if (srcW <= 0 || srcH <= 0) { delete pImg; return; }

    float scaleX = static_cast<float>(dstW) / srcW;
    float scaleY = static_cast<float>(dstH) / srcH;
    float scale  = min(scaleX, scaleY);
    int   drawW  = static_cast<int>(srcW * scale);
    int   drawH  = static_cast<int>(srcH * scale);
    int   offX   = (dstW - drawW) / 2;
    int   offY   = (dstH - drawH) / 2;

    CDC* pWndDC = pPreviewWnd->GetDC();
    if (!pWndDC) { delete pImg; return; }

    CDC memDC;
    memDC.CreateCompatibleDC(pWndDC);
    m_bitmap.CreateCompatibleBitmap(pWndDC, dstW, dstH);
    CBitmap* pOld = memDC.SelectObject(&m_bitmap);

    CRect rcFill(0, 0, dstW, dstH);
    memDC.FillSolidRect(&rcFill, RGB(225, 248, 242));

    Graphics g(memDC.GetSafeHdc());
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.DrawImage(pImg, offX, offY, drawW, drawH);

    memDC.SelectObject(pOld);
    pPreviewWnd->ReleaseDC(pWndDC);
    delete pImg;

    m_bHasPhoto = true;
    pPreviewWnd->Invalidate(FALSE);
    pPreviewWnd->UpdateWindow();
    Invalidate(FALSE);
}

void DeliveryPhotoDlg::OnPaint()
{
    CDialogEx::OnPaint();

    if (!m_bHasPhoto || !m_bitmap.GetSafeHandle()) return;

    CWnd* pPreviewWnd = GetDlgItem(IDC_STATIC_QR_VIEW);
    if (!pPreviewWnd) return;

    CClientDC dc(pPreviewWnd);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap* pOld = memDC.SelectObject(&m_bitmap);

    CRect rc;
    pPreviewWnd->GetClientRect(&rc);
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOld);
}

void DeliveryPhotoDlg::OnBtnSelectPhoto()
{
    CFileDialog dlg(TRUE, _T("jpg"), nullptr,
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
        _T("Image Files (*.jpg;*.jpeg;*.png)|*.jpg;*.jpeg;*.png||"), this);
    if (dlg.DoModal() != IDOK) return;

    m_strPhotoPath = dlg.GetPathName();
    SetDlgItemText(IDC_STATIC_PHOTO_PATH, m_strPhotoPath);
    LoadAndShowPhoto(m_strPhotoPath);
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_SKIP_PHOTO)->EnableWindow(FALSE);
}

void DeliveryPhotoDlg::OnBtnSkipPhoto()
{
    m_strPhotoPath.Empty();
    m_bHasPhoto = false;
    if (m_bitmap.GetSafeHandle())
        m_bitmap.DeleteObject();
    SetDlgItemText(IDC_STATIC_PHOTO_PATH, _T(""));
    CWnd* pPreviewWnd = GetDlgItem(IDC_STATIC_QR_VIEW);
    if (pPreviewWnd) { pPreviewWnd->Invalidate(TRUE); pPreviewWnd->UpdateWindow(); }
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
}

void DeliveryPhotoDlg::OnBtnConfirmPhoto()
{
    int orderId = AppContext::Get().currentOrder.orderId;
    if (orderId <= 0) {
        MessageBox(_T("Invalid order."), _T("Error"), MB_OK | MB_ICONWARNING);
        return;
    }
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(FALSE);

    json req;
    req["order_id"] = orderId;
    if (!m_strPhotoPath.IsEmpty()) {
        CT2A pathUtf8(m_strPhotoPath, CP_UTF8);
        req["photo_path"] = std::string(pathUtf8);
    }
    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_DELIVERY_DONE, req.dump());
    if (!bSent) {
        MessageBox(_T("Delivery complete! Great job."), _T("Done"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
    }
}

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
            int fee = res.value("delivery_fee", 0);
            CString msg;
            msg.Format(_T("Delivery complete!\nDelivery fee: %d won"), fee);
            MessageBox(msg, _T("Done"), MB_OK | MB_ICONINFORMATION);
            EndDialog(IDOK);
        } else {
            std::string err = res.value("message", "Server error.");
            CA2T wErr(err.c_str(), CP_UTF8);
            MessageBox(CString(wErr), _T("Error"), MB_OK | MB_ICONWARNING);
            GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
        }
    } catch (...) {}
    return 0;
}

HBRUSH DeliveryPhotoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        if (!m_hBrushBg) m_hBrushBg = CreateSolidBrush(RGB(225, 248, 242));
        pDC->SetBkColor(RGB(225, 248, 242));
        pDC->SetTextColor(RGB(10, 10, 10));
        return m_hBrushBg;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(10, 10, 10));
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    return hbr;
}
