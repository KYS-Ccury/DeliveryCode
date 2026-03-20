// DeliveryPhotoDlg.cpp  –  전달 사진 선택(CFileDialog) + 전달완료
#include "pch.h"
#include "DeliveryPhotoDlg.h"
#include "Protocol.h"
#include "AppContext.h"

IMPLEMENT_DYNAMIC(DeliveryPhotoDlg, CDialogEx)

BEGIN_MESSAGE_MAP(DeliveryPhotoDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_SELECT_PHOTO,  &DeliveryPhotoDlg::OnBtnSelectPhoto)
    ON_BN_CLICKED(IDC_BTN_SKIP_PHOTO,    &DeliveryPhotoDlg::OnBtnSkipPhoto)
    ON_BN_CLICKED(IDC_BTN_CONFIRM_PHOTO, &DeliveryPhotoDlg::OnBtnConfirmPhoto)
    ON_MESSAGE(WM_SOCKET_RECV,           &DeliveryPhotoDlg::OnSocketRecv)
END_MESSAGE_MAP()

DeliveryPhotoDlg::DeliveryPhotoDlg(CWnd* pParent)
    : CDialogEx(IDD_DELIVERY_PHOTO_DLG, pParent)
{
}

DeliveryPhotoDlg::~DeliveryPhotoDlg()
{
}

BOOL DeliveryPhotoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    AppContext::Get().socket.SetNotifyWnd(GetSafeHwnd());

    // 전달 완료 버튼 비활성화 (사진 첨부 또는 건너뛰기 선택 전)
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(FALSE);

    return TRUE;
}

// ─────────────────────────────────────────────
// 사진 선택 (CFileDialog)
// ─────────────────────────────────────────────
void DeliveryPhotoDlg::OnBtnSelectPhoto()
{
    CFileDialog dlg(TRUE,           // bOpenFileDialog
                    _T("jpg"),      // lpszDefExt
                    nullptr,        // lpszFileName
                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("이미지 파일 (*.jpg;*.jpeg;*.png)|*.jpg;*.jpeg;*.png|모든 파일 (*.*)|*.*||"),
                    this);

    if (dlg.DoModal() == IDOK) {
        m_strPhotoPath = dlg.GetPathName();
        SetDlgItemText(IDC_STATIC_PHOTO_PATH, m_strPhotoPath);
        SetDlgItemText(IDC_STATIC_QR_VIEW,    _T("[사진 첨부됨]\n") + m_strPhotoPath);
        GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
    }
}

// ─────────────────────────────────────────────
// 건너뛰기 (사진 없이 전달 완료)
// ─────────────────────────────────────────────
void DeliveryPhotoDlg::OnBtnSkipPhoto()
{
    m_strPhotoPath.Empty();
    SetDlgItemText(IDC_STATIC_PHOTO_PATH, _T("(사진 없음)"));
    SetDlgItemText(IDC_STATIC_QR_VIEW,    _T("(사진 없이 완료)"));
    GetDlgItem(IDC_BTN_CONFIRM_PHOTO)->EnableWindow(TRUE);
}

// ─────────────────────────────────────────────
// 전달 완료 확인
// ─────────────────────────────────────────────
void DeliveryPhotoDlg::OnBtnConfirmPhoto()
{
    // 서버에 배달 완료 패킷 전송
    // 실제로는 사진 파일을 바이너리로 전송하지만,
    // 현재는 파일 경로만 전송 (서버 연결 시 확장)
    CString payload;
    payload.Format(_T("%d|%s"),
                   AppContext::Get().currentOrder.orderId,
                   m_strPhotoPath.IsEmpty() ? _T("NOPHOTO")
                                            : static_cast<LPCTSTR>(m_strPhotoPath));

    bool bSent = AppContext::Get().socket.SendPacket(CMD_RIDER_DELIVERY_DONE, payload);

    if (!bSent) {
        // 서버 미연결 시 즉시 완료 처리
        MessageBox(_T("전달이 완료되었습니다!\n수고하셨습니다."),
                   _T("전달 완료"), MB_OK | MB_ICONINFORMATION);
        EndDialog(IDOK);
        return;
    }
    // 서버 응답은 OnSocketRecv에서 처리
}

// ─────────────────────────────────────────────
// 서버 응답: "404|OK" → 배달 완료 확인
// ─────────────────────────────────────────────
LRESULT DeliveryPhotoDlg::OnSocketRecv(WPARAM /*w*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (!pMsg) return 0;
    CString msg = *pMsg;
    delete pMsg;

    int p = msg.Find(_T('|'));
    if (p < 0) return 0;
    int cmd = _ttoi(msg.Left(p));

    if (cmd == CMD_RIDER_DELIVERY_DONE) {
        CString rest = msg.Mid(p + 1);
        if (rest.Left(2) == _T("OK")) {
            MessageBox(_T("전달이 완료되었습니다!\n수고하셨습니다."),
                       _T("전달 완료"), MB_OK | MB_ICONINFORMATION);
            EndDialog(IDOK);
        } else {
            MessageBox(_T("서버 처리 중 오류가 발생했습니다.\n다시 시도해주세요."),
                       _T("오류"), MB_OK | MB_ICONWARNING);
        }
    }
    return 0;
}

void DeliveryPhotoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}
HBRUSH DeliveryPhotoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    pDC->SetBkColor(RGB(225, 248, 242));
    pDC->SetTextColor(RGB(30, 60, 50));
    return m_hBrushBg;
}
