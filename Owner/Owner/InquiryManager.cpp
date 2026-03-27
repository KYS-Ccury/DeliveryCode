#include "pch.h"
#include "InquiryManager.h"
#include "CInquiryDlg.h"

void CInquiryManager::HandleInquiryClick(CWnd* pParent)
{
    if (!pParent) return;

    // 메세지박스 지우고 팝업창 띄우기!
    CInquiryDlg dlg(pParent);
    dlg.DoModal();
}